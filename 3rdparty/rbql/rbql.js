(function(exports){
// The magic line above is to make the module both browser and Node compatible, see https://stackoverflow.com/questions/3225251/how-can-i-share-code-between-node-js-and-the-browser

// This module works with records only. It is CSV-agnostic.
// Do not add CSV-related logic or variables/functions/objects like "delim", "separator" etc


class RbqlParsingError extends Error {}
class RbqlRuntimeError extends Error {}
class AssertionError extends Error {}
class RbqlIOHandlingError extends Error {}


class InternalBadFieldError extends Error {
    constructor(bad_idx, ...params) {
        super(...params);
        this.bad_idx = bad_idx;
    }
}


function assert(condition, message=null) {
    if (!condition) {
        if (!message) {
            message = 'Assertion error';
        }
        throw new AssertionError(message);
    }
}


function replace_all(src, search, replacement) {
    return src.split(search).join(replacement);
}


class RBQLContext {
    constructor(query_text, input_iterator, output_writer, user_init_code) {
        this.query_text = query_text;
        this.input_iterator = input_iterator;
        this.writer = output_writer;
        this.user_init_code = user_init_code;

        this.unnest_list = null;
        this.top_count = null;

        this.like_regex_cache = new Map();

        this.sort_key_expression = null;

        this.aggregation_stage = 0;
        this.aggregation_key_expression = null;
        this.functional_aggregators = [];

        this.join_map_impl = null;
        this.join_map = null;
        this.lhs_join_var_expression = null;

        this.where_expression = null;

        this.select_expression = null;

        this.update_expressions = null;

        this.variables_init_code = null;
    }
}

var query_context = null; // Needs to be global for MIN(), MAX(), etc functions. TODO find a way to make it local.


const wrong_aggregation_usage_error = 'Usage of RBQL aggregation functions inside JavaScript expressions is not allowed, see the docs';
const RBQL_VERSION = '0.27.0';


function check_if_brackets_match(opening_bracket, closing_bracket) {
    return (opening_bracket == '[' && closing_bracket == ']') || (opening_bracket == '(' && closing_bracket == ')') || (opening_bracket == '{' && closing_bracket == '}');
}


function parse_root_bracket_level_text_spans(select_expression) {
    let text_spans = []; // parts of text separated by commas at the root parenthesis level
    let last_pos = 0;
    let bracket_stack = [];
    for (let i = 0; i < select_expression.length; i++) {
        let cur_char = select_expression[i];
        if (cur_char == ',' && bracket_stack.length == 0) {
            text_spans.push(select_expression.substring(last_pos, i));
            last_pos = i + 1;
        } else if (['[', '{', '('].indexOf(cur_char) != -1) {
            bracket_stack.push(cur_char);
        } else if ([']', '}', ')'].indexOf(cur_char) != -1) {
            if (bracket_stack.length && check_if_brackets_match(bracket_stack[bracket_stack.length - 1], cur_char)) {
                bracket_stack.pop();
            } else {
                throw new RbqlParsingError(`Unable to parse column headers in SELECT expression: No matching opening bracket for closing "${cur_char}"`);
            }
        }
    }
    if (bracket_stack.length) {
        throw new RbqlParsingError(`Unable to parse column headers in SELECT expression: No matching closing bracket for opening "${bracket_stack[0]}"`);
    }
    text_spans.push(select_expression.substring(last_pos, select_expression.length));
    text_spans = text_spans.map(span => span.trim());
    return text_spans;
}


function unquote_string(quoted_str) {
    // It's possible to use eval here to unqoute the quoted_column_name, but it would be a little barbaric, let's do it manually instead
    if (!quoted_str || quoted_str.length < 2)
        return null;
    if (quoted_str[0] == "'" && quoted_str[quoted_str.length - 1] == "'") {
        return quoted_str.substring(1, quoted_str.length - 1).replace(/\\'/g, "'").replace(/\\\\/g, "\\");
    } else if (quoted_str[0] == '"' && quoted_str[quoted_str.length - 1] == '"') {
        return quoted_str.substring(1, quoted_str.length - 1).replace(/\\"/g, '"').replace(/\\\\/g, "\\");
    } else {
        return null;
    }
}


function column_info_from_text_span(text_span, string_literals) {
    // This function is a rough equivalent of "column_info_from_node()" function in python version of RBQL
    text_span = text_span.trim();
    let rbql_star_marker = '__RBQL_INTERNAL_STAR';
    let simple_var_match = /^[_a-zA-Z][_a-zA-Z0-9]*$/.exec(text_span);
    let attribute_match = /^([ab])\.([_a-zA-Z][_a-zA-Z0-9]*)$/.exec(text_span);
    let subscript_int_match = /^([ab])\[([0-9]+)\]$/.exec(text_span);
    let subscript_str_match = /^([ab])\[___RBQL_STRING_LITERAL([0-9]+)___\]$/.exec(text_span);
    let as_alias_match = /^(.*) (as|AS) +([a-zA-Z][a-zA-Z0-9_]*) *$/.exec(text_span);
    if (as_alias_match !== null) {
        return {table_name: null, column_index: null, column_name: null, is_star: false, alias_name: as_alias_match[3]};
    }
    if (simple_var_match !== null) {
        if (text_span == rbql_star_marker)
            return {table_name: null, column_index: null, column_name: null, is_star: true, alias_name: null};
        if (text_span.startsWith('___RBQL_STRING_LITERAL'))
            return null;
        let match = /^([ab])([0-9]+)$/.exec(text_span);
        if (match !== null) {
            return {table_name: match[1], column_index: parseInt(match[2]) - 1, column_name: null, is_star: false, alias_name: null};
        }
        // Some examples for this branch: NR, NF
        return {table_name: null, column_index: null, column_name: text_span, is_star: false, alias_name: null};
    } else if (attribute_match !== null) {
        let table_name = attribute_match[1];
        let column_name = attribute_match[2];
        if (column_name == rbql_star_marker) {
            return {table_name: table_name, column_index: null, column_name: null, is_star: true, alias_name: null};
        }
        return {table_name: null, column_index: null, column_name: column_name, is_star: false, alias_name: null};
    } else if (subscript_int_match != null) {
        let table_name = subscript_int_match[1];
        let column_index = parseInt(subscript_int_match[2]) - 1;
        return {table_name: table_name, column_index: column_index, column_name: null, is_star: false, alias_name: null};
    } else if (subscript_str_match != null) {
        let table_name = subscript_str_match[1];
        let replaced_string_literal_id = subscript_str_match[2];
        if (replaced_string_literal_id < string_literals.length) {
            let quoted_column_name = string_literals[replaced_string_literal_id];
            let unquoted_column_name = unquote_string(quoted_column_name);
            if (unquoted_column_name !== null && unquoted_column_name !== undefined) {
                return {table_name: null, column_index: null, column_name: unquoted_column_name, is_star: false, alias_name: null};
            }
        }
    }
    return null;
}


function adhoc_parse_select_expression_to_column_infos(select_expression, string_literals) {
    // It is acceptable for the algorithm to provide null column name when it could be theorethically possible to deduce the name.
    // I.e. this algorithm guarantees precision but doesn't guarantee completeness in all theorethically possible queries.
    // Although the algorithm should be complete in all practical scenarios, i.e. it should be hard to come up with the query that doesn't produce complete set of column names.
    // The null column name just means that the output column will be named as col{i}, so the failure to detect the proper column name can be tolerated.
    // Specifically this function guarantees the following:
    // 1. The number of column_infos is correct and will match the number of fields in each record in the output - otherwise the exception should be thrown
    // 2. If column_info at pos j is not null, it is guaranteed to correctly represent that column name in the output
    let text_spans = parse_root_bracket_level_text_spans(select_expression);
    let column_infos = text_spans.map(ts => column_info_from_text_span(ts, string_literals));
    return column_infos;
}


function stable_compare(a, b) {
    for (var i = 0; i < a.length; i++) {
        if (a[i] !== b[i])
            return a[i] < b[i] ? -1 : 1;
    }
}


function safe_get(record, idx) {
    return idx < record.length ? record[idx] : null;
}


function safe_join_get(record, idx) {
    if (idx < record.length) {
        return record[idx];
    }
    throw new InternalBadFieldError(idx);
}


function safe_set(record, idx, value) {
    if (idx < record.length) {
        record[idx] = value;
    } else {
        throw new InternalBadFieldError(idx);
    }
}


function regexp_escape(text) {
    // From here: https://stackoverflow.com/a/6969486/2898283
    return text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');  // $& means the whole matched text
}


function like_to_regex(pattern) {
    let p = 0;
    let i = 0;
    let converted = '';
    while (i < pattern.length) {
        if (pattern.charAt(i) == '_' || pattern.charAt(i) == '%') {
            converted += regexp_escape(pattern.substring(p, i));
            p = i + 1;
            if (pattern.charAt(i) == '_') {
                converted += '.';
            } else {
                converted += '.*';
            }
        }
        i += 1;
    }
    converted += regexp_escape(pattern.substring(p, i));
    return '^' + converted + '$';
}


function like(text, pattern) {
    let matcher = query_context.like_regex_cache.get(pattern);
    if (matcher === undefined) {
        matcher = new RegExp(like_to_regex(pattern));
        query_context.like_regex_cache.set(pattern, matcher);
    }
    return matcher.test(text);
}
const LIKE = like;


class RBQLAggregationToken {
    constructor(marker_id, value) {
        this.marker_id = marker_id;
        this.value = value;
    }

    toString() {
        throw new RbqlParsingError(wrong_aggregation_usage_error);
    }
}


class UnnestMarker {}


function UNNEST(vals) {
    if (query_context.unnest_list !== null) {
        // Technically we can support multiple UNNEST's but the implementation/algorithm is more complex and just doesn't worth it
        throw new RbqlParsingError('Only one UNNEST is allowed per query');
    }
    query_context.unnest_list = vals;
    return new UnnestMarker();
}
const unnest = UNNEST;
const Unnest = UNNEST;
const UNFOLD = UNNEST; // "UNFOLD" is deprecated, just for backward compatibility


function parse_number(val) {
    // We can do a more pedantic number test like `/^ *-{0,1}[0-9]+\.{0,1}[0-9]* *$/.test(val)`, but  user will probably use just Number(val) or parseInt/parseFloat
    let result = Number(val);
    if (isNaN(result)) {
        throw new RbqlRuntimeError(`Unable to convert value "${val}" to a number. MIN, MAX, SUM, AVG, MEDIAN and VARIANCE aggregate functions convert their string arguments to numeric values`);
    }
    return result;
}


class AnyValueAggregator {
    constructor() {
        this.stats = new Map();
    }

    increment(key, val) {
        var cur_aggr = this.stats.get(key);
        if (cur_aggr === undefined) {
            this.stats.set(key, val);
        }
    }

    get_final(key) {
        return this.stats.get(key);
    }
}


class MinAggregator {
    constructor() {
        this.stats = new Map();
    }

    increment(key, val) {
        val = parse_number(val);
        var cur_aggr = this.stats.get(key);
        if (cur_aggr === undefined) {
            this.stats.set(key, val);
        } else {
            this.stats.set(key, Math.min(cur_aggr, val));
        }
    }

    get_final(key) {
        return this.stats.get(key);
    }
}


class MaxAggregator {
    constructor() {
        this.stats = new Map();
    }

    increment(key, val) {
        val = parse_number(val);
        var cur_aggr = this.stats.get(key);
        if (cur_aggr === undefined) {
            this.stats.set(key, val);
        } else {
            this.stats.set(key, Math.max(cur_aggr, val));
        }
    }

    get_final(key) {
        return this.stats.get(key);
    }
}


class SumAggregator {
    constructor() {
        this.stats = new Map();
    }

    increment(key, val) {
        val = parse_number(val);
        var cur_aggr = this.stats.get(key);
        if (cur_aggr === undefined) {
            this.stats.set(key, val);
        } else {
            this.stats.set(key, cur_aggr + val);
        }
    }

    get_final(key) {
        return this.stats.get(key);
    }
}


class AvgAggregator {
    constructor() {
        this.stats = new Map();
    }

    increment(key, val) {
        val = parse_number(val);
        var cur_aggr = this.stats.get(key);
        if (cur_aggr === undefined) {
            this.stats.set(key, [val, 1]);
        } else {
            var cur_sum = cur_aggr[0];
            var cur_cnt = cur_aggr[1];
            this.stats.set(key, [cur_sum + val, cur_cnt + 1]);
        }
    }

    get_final(key) {
        var cur_aggr = this.stats.get(key);
        var cur_sum = cur_aggr[0];
        var cur_cnt = cur_aggr[1];
        var avg = cur_sum / cur_cnt;
        return avg;
    }
}


class VarianceAggregator {
    constructor() {
        this.stats = new Map();
    }

    increment(key, val) {
        val = parse_number(val);
        var cur_aggr = this.stats.get(key);
        if (cur_aggr === undefined) {
            this.stats.set(key, [val, val * val, 1]);
        } else {
            var cur_sum = cur_aggr[0];
            var cur_sum_sq = cur_aggr[1];
            var cur_cnt = cur_aggr[2];
            this.stats.set(key, [cur_sum + val, cur_sum_sq + val * val, cur_cnt + 1]);
        }
    }

    get_final(key) {
        var cur_aggr = this.stats.get(key);
        var cur_sum = cur_aggr[0];
        var cur_sum_sq = cur_aggr[1];
        var cur_cnt = cur_aggr[2];
        var avg_val = cur_sum / cur_cnt;
        var variance = cur_sum_sq / cur_cnt - avg_val * avg_val;
        return variance;
    }
}


class MedianAggregator {
    constructor() {
        this.stats = new Map();
    }

    increment(key, val) {
        val = parse_number(val);
        var cur_aggr = this.stats.get(key);
        if (cur_aggr === undefined) {
            this.stats.set(key, [val]);
        } else {
            cur_aggr.push(val);
        }
    }

    get_final(key) {
        var cur_aggr = this.stats.get(key);
        cur_aggr.sort(function(a, b) { return a - b; });
        var m = Math.floor(cur_aggr.length / 2);
        if (cur_aggr.length % 2) {
            return cur_aggr[m];
        } else {
            return (cur_aggr[m - 1] + cur_aggr[m]) / 2.0;
        }
    }
}


class CountAggregator {
    constructor() {
        this.stats = new Map();
    }

    increment(key, val) {
        var cur_aggr = this.stats.get(key);
        if (cur_aggr === undefined) {
            this.stats.set(key, 1);
        } else {
            this.stats.set(key, cur_aggr + 1);
        }
    }

    get_final(key) {
        return this.stats.get(key);
    }
}


class ArrayAggAggregator {
    constructor(post_proc=null) {
        this.post_proc = post_proc;
        this.stats = new Map();
    }

    increment(key, val) {
        let cur_aggr = this.stats.get(key);
        if (cur_aggr === undefined) {
            this.stats.set(key, [val]);
        } else {
            cur_aggr.push(val);
        }
    }

    get_final(key) {
        let cur_aggr = this.stats.get(key);
        if (this.post_proc === null)
            return cur_aggr;
        return this.post_proc(cur_aggr);
    }
}


class ConstGroupVerifier {
    constructor(output_index) {
        this.output_index = output_index;
        this.const_values = new Map();
    }

    increment(key, value) {
        var old_value = this.const_values.get(key);
        if (old_value === undefined) {
            this.const_values.set(key, value);
        } else if (old_value != value) {
            throw new RbqlRuntimeError(`Invalid aggregate expression: non-constant values in output column ${this.output_index + 1}. E.g. "${old_value}" and "${value}"`);
        }
    }

    get_final(key) {
        return this.const_values.get(key);
    }
}


function init_aggregator(generator_name, val, post_proc=null) {
    query_context.aggregation_stage = 1;
    var res = new RBQLAggregationToken(query_context.functional_aggregators.length, val);
    if (post_proc === null) {
        query_context.functional_aggregators.push(new generator_name());
    } else {
        query_context.functional_aggregators.push(new generator_name(post_proc));
    }
    return res;
}

function ANY_VALUE(val) {
    return query_context.aggregation_stage < 2 ? init_aggregator(AnyValueAggregator, val) : val;
}
const any_value = ANY_VALUE;
const Any_value = ANY_VALUE;

function MIN(val) {
    return query_context.aggregation_stage < 2 ? init_aggregator(MinAggregator, val) : val;
}
const min = MIN;
const Min = MIN;

function MAX(val) {
    return query_context.aggregation_stage < 2 ? init_aggregator(MaxAggregator, val) : val;
}
const max = MAX;
const Max = MAX;

function COUNT(val) {
    return query_context.aggregation_stage < 2 ? init_aggregator(CountAggregator, 1) : 1;
}
const count = COUNT;
const Count = COUNT;

function SUM(val) {
    return query_context.aggregation_stage < 2 ? init_aggregator(SumAggregator, val) : val;
}
const sum = SUM;
const Sum = SUM;

function AVG(val) {
    return query_context.aggregation_stage < 2 ? init_aggregator(AvgAggregator, val) : val;
}
const avg = AVG;
const Avg = AVG;

function VARIANCE(val) {
    return query_context.aggregation_stage < 2 ? init_aggregator(VarianceAggregator, val) : val;
}
const variance = VARIANCE;
const Variance = VARIANCE;

function MEDIAN(val) {
    return query_context.aggregation_stage < 2 ? init_aggregator(MedianAggregator, val) : val;
}
const median = MEDIAN;
const Median = MEDIAN;

function ARRAY_AGG(val, post_proc=null) {
    return query_context.aggregation_stage < 2 ? init_aggregator(ArrayAggAggregator, val, post_proc) : val;
}
const array_agg = ARRAY_AGG;
const FOLD = ARRAY_AGG; // "FOLD" is deprecated, just for backward compatibility


function add_to_set(dst_set, value) {
    var len_before = dst_set.size;
    dst_set.add(value);
    return len_before != dst_set.size;
}


class TopWriter {
    constructor(subwriter, top_count) {
        this.subwriter = subwriter;
        this.NW = 0;
        this.top_count = top_count;
    }

    write(record) {
        if (this.top_count !== null && this.NW >= this.top_count)
            return false;
        this.subwriter.write(record);
        this.NW += 1;
        return true;
    }

    finish() {
        this.subwriter.finish();
    }
}


class UniqWriter {
    constructor(subwriter) {
        this.subwriter = subwriter;
        this.seen = new Set();
    }

    write(record) {
        if (!add_to_set(this.seen, JSON.stringify(record)))
            return true;
        if (!this.subwriter.write(record))
            return false;
        return true;
    }

    finish() {
        this.subwriter.finish();
    }
}


class UniqCountWriter {
    constructor(subwriter) {
        this.subwriter = subwriter;
        this.records = new Map();
    }

    write(record) {
        var key = JSON.stringify(record);
        var old_val = this.records.get(key);
        if (old_val) {
            old_val[0] += 1;
        } else {
            this.records.set(key, [1, record]);
        }
        return true;
    }

    finish() {
        for (var [key, value] of this.records) {
            let [count, record] = value;
            record.unshift(count);
            if (!this.subwriter.write(record))
                break;
        }
        this.subwriter.finish();
    }
}


class SortedWriter {
    constructor(subwriter, reverse_sort) {
        this.subwriter = subwriter;
        this.reverse_sort = reverse_sort;
        this.unsorted_entries = [];
    }

    write(stable_entry) {
        this.unsorted_entries.push(stable_entry);
        return true;
    }

    finish() {
        var unsorted_entries = this.unsorted_entries;
        unsorted_entries.sort(stable_compare);
        if (this.reverse_sort)
            unsorted_entries.reverse();
        for (var i = 0; i < unsorted_entries.length; i++) {
            var entry = unsorted_entries[i];
            if (!this.subwriter.write(entry[entry.length - 1]))
                break;
        }
        this.subwriter.finish();
    }
}


class AggregateWriter {
    constructor(subwriter) {
        this.subwriter = subwriter;
        this.aggregators = [];
        this.aggregation_keys = new Set();
    }

    finish() {
        var all_keys = Array.from(this.aggregation_keys);
        all_keys.sort();
        for (var i = 0; i < all_keys.length; i++) {
            var key = all_keys[i];
            var out_fields = [];
            for (var ag of this.aggregators) {
                out_fields.push(ag.get_final(key));
            }
            if (!this.subwriter.write(out_fields))
                break;
        }
        this.subwriter.finish();
    }
}


class InnerJoiner {
    constructor(join_map) {
        this.join_map = join_map;
    }

    get_rhs(lhs_key) {
        return this.join_map.get_join_records(lhs_key);
    }
}


class LeftJoiner {
    constructor(join_map) {
        this.join_map = join_map;
        this.null_record = [[null, join_map.max_record_len, Array(join_map.max_record_len).fill(null)]];
    }

    get_rhs(lhs_key) {
        let result = this.join_map.get_join_records(lhs_key);
        if (result.length == 0) {
            return this.null_record;
        }
        return result;
    }
}


class StrictLeftJoiner {
    constructor(join_map) {
        this.join_map = join_map;
    }

    get_rhs(lhs_key) {
        let result = this.join_map.get_join_records(lhs_key);
        if (result.length != 1) {
            throw new RbqlRuntimeError('In "STRICT LEFT JOIN" each key in A must have exactly one match in B. Bad A key: "' + lhs_key + '"');
        }
        return result;
    }
}


function select_except(src, except_fields) {
    let result = [];
    for (let i = 0; i < src.length; i++) {
        if (except_fields.indexOf(i) == -1)
            result.push(src[i]);
    }
    return result;
}


function select_simple(sort_key, NR, out_fields) {
    if (query_context.sort_key_expression !== null) {
        var sort_entry = sort_key.concat([NR, out_fields]);
        if (!query_context.writer.write(sort_entry))
            return false;
    } else {
        if (!query_context.writer.write(out_fields))
            return false;
    }
    return true;
}


function select_aggregated(key, transparent_values) {
    if (key !== null) {
        key = JSON.stringify(key);
    }
    if (query_context.aggregation_stage === 1) {
        if (!(query_context.writer instanceof TopWriter)) {
            throw new RbqlParsingError('"ORDER BY", "UPDATE" and "DISTINCT" keywords are not allowed in aggregate queries');
        }
        query_context.writer = new AggregateWriter(query_context.writer);
        let num_aggregators_found = 0;
        for (var i = 0; i < transparent_values.length; i++) {
            var trans_value = transparent_values[i];
            if (trans_value instanceof RBQLAggregationToken) {
                query_context.writer.aggregators.push(query_context.functional_aggregators[trans_value.marker_id]);
                query_context.writer.aggregators[query_context.writer.aggregators.length - 1].increment(key, trans_value.value);
                num_aggregators_found += 1;
            } else {
                query_context.writer.aggregators.push(new ConstGroupVerifier(query_context.writer.aggregators.length));
                query_context.writer.aggregators[query_context.writer.aggregators.length - 1].increment(key, trans_value);
            }
        }
        if (num_aggregators_found != query_context.functional_aggregators.length) {
            throw new RbqlParsingError(wrong_aggregation_usage_error);
        }
        query_context.aggregation_stage = 2;
    } else {
        for (var i = 0; i < transparent_values.length; i++) {
            var trans_value = transparent_values[i];
            query_context.writer.aggregators[i].increment(key, trans_value);
        }
    }
    query_context.writer.aggregation_keys.add(key);
}


function select_unnested(sort_key, NR, folded_fields) {
    let out_fields = folded_fields.slice();
    let unnest_pos = folded_fields.findIndex(val => val instanceof UnnestMarker);
    for (var i = 0; i < query_context.unnest_list.length; i++) {
        out_fields[unnest_pos] = query_context.unnest_list[i];
        if (!select_simple(sort_key, NR, out_fields.slice()))
            return false;
    }
    return true;
}


const PROCESS_SELECT_COMMON = `
__RBQLMP__variables_init_code
if (__RBQLMP__where_expression) {
    let out_fields = __RBQLMP__select_expression;
    if (query_context.aggregation_stage > 0) {
        let key = __RBQLMP__aggregation_key_expression;
        select_aggregated(key, out_fields);
    } else {
        let sort_key = [__RBQLMP__sort_key_expression];
        if (query_context.unnest_list !== null) {
            if (!select_unnested(sort_key, NR, out_fields))
                stop_flag = true;
        } else {
            if (!select_simple(sort_key, NR, out_fields))
                stop_flag = true;
        }
    }
}
`;


const PROCESS_SELECT_SIMPLE = `
let star_fields = record_a;
__CODE__
`;


const PROCESS_SELECT_JOIN = `
let join_matches = query_context.join_map.get_rhs(__RBQLMP__lhs_join_var_expression);
for (let join_match of join_matches) {
    let [bNR, bNF, record_b] = join_match;
    let star_fields = record_a.concat(record_b);
    __CODE__
    if (stop_flag)
        break;
}
`;


const PROCESS_UPDATE_JOIN = `
let join_matches = query_context.join_map.get_rhs(__RBQLMP__lhs_join_var_expression);
if (join_matches.length > 1)
    throw new RbqlRuntimeError('More than one record in UPDATE query matched a key from the input table in the join table');
let record_b = null;
let bNR = null;
let bNF = null;
if (join_matches.length == 1)
    [bNR, bNF, record_b] = join_matches[0];
let up_fields = record_a;
__RBQLMP__variables_init_code
if (join_matches.length == 1 && (__RBQLMP__where_expression)) {
    NU += 1;
    __RBQLMP__update_expressions
}
if (!query_context.writer.write(up_fields))
    stop_flag = true;
`;


const PROCESS_UPDATE_SIMPLE = `
let up_fields = record_a;
__RBQLMP__variables_init_code
if (__RBQLMP__where_expression) {
    NU += 1;
    __RBQLMP__update_expressions
}
if (!query_context.writer.write(up_fields))
    stop_flag = true;
`;


const MAIN_LOOP_BODY = `
__USER_INIT_CODE__

let NU = 0;
let NR = 0;

let stop_flag = false;
while (!stop_flag) {
    let record_a = query_context.input_iterator.get_record();
    if (record_a === null)
        break;
    NR += 1;
    let NF = record_a.length;
    query_context.unnest_list = null; // TODO optimize, don't need to set this every iteration
    try {
        __CODE__
    } catch (e) {
        if (e.constructor.name === 'InternalBadFieldError') {
            throw new RbqlRuntimeError(\`No "a\${e.bad_idx + 1}" field at record \${NR}\`);
        } else if (e.constructor.name === 'RbqlParsingError') {
            throw(e);
        } else {
            throw new RbqlRuntimeError(\`At record \${NR}, Details: \${e.message}\`);
        }
    }
}
`;


function embed_expression(parent_code, child_placeholder, child_expression) {
    return replace_all(parent_code, child_placeholder, child_expression);
}


function embed_code(parent_code, child_placeholder, child_code) {
    let parent_lines = parent_code.split('\n');
    let child_lines = child_code.split('\n');
    for (let i = 0; i < parent_lines.length; i++) {
        let pos = parent_lines[i].indexOf(child_placeholder);
        if (pos == -1)
            continue;
        assert(pos % 4 == 0);
        let placeholder_indentation = parent_lines[i].substring(0, pos);
        child_lines = child_lines.map(l => placeholder_indentation + l);
        let result_lines = parent_lines.slice(0, i).concat(child_lines).concat(parent_lines.slice(i + 1));
        return result_lines.join('\n') + '\n';
    }
    assert(false);
}


function generate_main_loop_code(query_context) {
    let is_select_query = query_context.select_expression !== null;
    let is_join_query = query_context.join_map !== null;
    let where_expression = query_context.where_expression === null ? 'true' : query_context.where_expression;
    let aggregation_key_expression = query_context.aggregation_key_expression === null ? 'null' : query_context.aggregation_key_expression;
    let sort_key_expression = query_context.sort_key_expression === null ? 'null' : query_context.sort_key_expression;
    let js_code = embed_code(MAIN_LOOP_BODY, '__USER_INIT_CODE__', query_context.user_init_code);
    if (is_select_query) {
        if (is_join_query) {
            js_code = embed_code(embed_code(js_code, '__CODE__', PROCESS_SELECT_JOIN), '__CODE__', PROCESS_SELECT_COMMON);
            js_code = embed_expression(js_code, '__RBQLMP__lhs_join_var_expression', query_context.lhs_join_var_expression);
        } else {
            js_code = embed_code(embed_code(js_code, '__CODE__', PROCESS_SELECT_SIMPLE), '__CODE__', PROCESS_SELECT_COMMON);
        }
        js_code = embed_code(js_code, '__RBQLMP__variables_init_code', query_context.variables_init_code);
        js_code = embed_expression(js_code, '__RBQLMP__select_expression', query_context.select_expression);
        js_code = embed_expression(js_code, '__RBQLMP__where_expression', where_expression);
        js_code = embed_expression(js_code, '__RBQLMP__aggregation_key_expression', aggregation_key_expression);
        js_code = embed_expression(js_code, '__RBQLMP__sort_key_expression', sort_key_expression);
    } else {
        if (is_join_query) {
            js_code = embed_code(js_code, '__CODE__', PROCESS_UPDATE_JOIN);
            js_code = embed_expression(js_code, '__RBQLMP__lhs_join_var_expression', query_context.lhs_join_var_expression);
        } else {
            js_code = embed_code(js_code, '__CODE__', PROCESS_UPDATE_SIMPLE);
        }
        js_code = embed_code(js_code, '__RBQLMP__variables_init_code', query_context.variables_init_code);
        js_code = embed_code(js_code, '__RBQLMP__update_expressions', query_context.update_expressions);
        js_code = embed_expression(js_code, '__RBQLMP__where_expression', where_expression);
    }
    return "(() => {" + js_code + "})()";
}


function compile_and_run(query_context) {
    let main_loop_body = generate_main_loop_code(query_context);
    try {
        let main_loop_promise = eval(main_loop_body);
        main_loop_promise;
    } catch (e) {
        if (e instanceof SyntaxError) {
            // SyntaxError's from eval() function do not contain detailed explanation of what has caused the syntax error, so to guess what was wrong we can only use the original query
            // v8 issue to fix eval: https://bugs.chromium.org/p/v8/issues/detail?id=2589
            let lower_case_query = query_context.query_text.toLowerCase();
            if (lower_case_query.indexOf(' having ') != -1)
                throw new SyntaxError(e.message + "\nRBQL doesn't support \"HAVING\" keyword");
            if (lower_case_query.indexOf(' like ') != -1)
                throw new SyntaxError(e.message + "\nRBQL doesn't support \"LIKE\" operator, use like() function instead e.g. ... WHERE like(a1, 'foo%bar') ... "); // UT JSON
            if (lower_case_query.indexOf(' from ') != -1)
                throw new SyntaxError(e.message + "\nTip: If input table is defined by the environment, RBQL query should not have \"FROM\" keyword"); // UT JSON
            if (e && e.message && String(e.message).toLowerCase().indexOf('unexpected identifier') != -1) {
                if (lower_case_query.indexOf(' and ') != -1)
                    throw new SyntaxError(e.message + "\nDid you use 'and' keyword in your query?\nJavaScript backend doesn't support 'and' keyword, use '&&' operator instead!");
                if (lower_case_query.indexOf(' or ') != -1)
                    throw new SyntaxError(e.message + "\nDid you use 'or' keyword in your query?\nJavaScript backend doesn't support 'or' keyword, use '||' operator instead!");
            }
        }
        if (e && e.message && e.message.indexOf('Received an instance of RBQLAggregationToken') != -1)
            throw new RbqlParsingError(wrong_aggregation_usage_error);
        throw e;
    }
}


const GROUP_BY = 'GROUP BY';
const UPDATE = 'UPDATE';
const SELECT = 'SELECT';
const JOIN = 'JOIN';
const INNER_JOIN = 'INNER JOIN';
const LEFT_JOIN = 'LEFT JOIN';
const LEFT_OUTER_JOIN = 'LEFT OUTER JOIN';
const STRICT_LEFT_JOIN = 'STRICT LEFT JOIN';
const ORDER_BY = 'ORDER BY';
const WHERE = 'WHERE';
const LIMIT = 'LIMIT';
const EXCEPT = 'EXCEPT';
const WITH = 'WITH';


function get_ambiguous_error_msg(variable_name) {
    return `Ambiguous variable name: "${variable_name}" is present both in input and in join tables`;
}


function get_all_matches(regexp, text) {
    var result = [];
    let match_obj = null;
    while((match_obj = regexp.exec(text)) !== null) {
        result.push(match_obj);
    }
    return result;
}


function str_strip(src) {
    return src.replace(/^ +| +$/g, '');
}


function strip_comments(cline) {
    cline = cline.trim();
    if (cline.startsWith('//'))
        return '';
    return cline;
}


function combine_string_literals(backend_expression, string_literals) {
    for (var i = 0; i < string_literals.length; i++) {
        backend_expression = replace_all(backend_expression, `___RBQL_STRING_LITERAL${i}___`, string_literals[i]);
    }
    return backend_expression;
}


function parse_basic_variables(query_text, prefix, dst_variables_map) {
    assert(prefix == 'a' || prefix == 'b');
    let rgx = new RegExp(`(?:^|[^_a-zA-Z0-9])${prefix}([1-9][0-9]*)(?:$|(?=[^_a-zA-Z0-9]))`, 'g');
    let matches = get_all_matches(rgx, query_text);
    for (let i = 0; i < matches.length; i++) {
        let field_num = parseInt(matches[i][1]);
        dst_variables_map[prefix + String(field_num)] = {initialize: true, index: field_num - 1};
    }
}


function parse_array_variables(query_text, prefix, dst_variables_map) {
    assert(prefix == 'a' || prefix == 'b');
    let rgx = new RegExp(`(?:^|[^_a-zA-Z0-9])${prefix}\\[([1-9][0-9]*)\\]`, 'g');
    let matches = get_all_matches(rgx, query_text);
    for (let i = 0; i < matches.length; i++) {
        let field_num = parseInt(matches[i][1]);
        dst_variables_map[`${prefix}[${field_num}]`] = {initialize: true, index: field_num - 1};
    }
}


function js_string_escape_column_name(column_name, quote_char) {
    column_name = column_name.replace(/\\/g, '\\\\');
    column_name = column_name.replace(/\n/g, '\\n');
    column_name = column_name.replace(/\r/g, '\\r');
    column_name = column_name.replace(/\t/g, '\\t');
    if (quote_char === "'")
        return column_name.replace(/'/g, "\\'");
    if (quote_char === '"')
        return column_name.replace(/"/g, '\\"');
    assert(quote_char === "`");
    return column_name.replace(/`/g, "\\`");
}


function query_probably_has_dictionary_variable(query_text, column_name) {
    let rgx = new RegExp('[-a-zA-Z0-9_:;+=!.,()%^#@&* ]+', 'g');
    let continuous_name_segments = get_all_matches(rgx, column_name);
    for (let continuous_segment of continuous_name_segments) {
        if (query_text.indexOf(continuous_segment) == -1)
            return false;
    }
    return true;
}


function parse_dictionary_variables(query_text, prefix, column_names, dst_variables_map) {
    // The purpose of this algorithm is to minimize number of variables in varibale_map to improve performance, ideally it should be only variables from the query

    // FIXME to prevent typos in attribute names either use query-based variable parsing which can properly handle back-tick strings or wrap "a" and "b" variables with ES6 Proxies https://stackoverflow.com/a/25658975/2898283
    assert(prefix === 'a' || prefix === 'b');
    let dict_test_rgx = new RegExp(`(?:^|[^_a-zA-Z0-9])${prefix}\\[`);
    if (query_text.search(dict_test_rgx) == -1)
        return;
    for (let i = 0; i < column_names.length; i++) {
        let column_name = column_names[i];
        if (query_probably_has_dictionary_variable(query_text, column_name)) {
            let escaped_column_name = js_string_escape_column_name(column_name, '"');
            dst_variables_map[`${prefix}["${escaped_column_name}"]`] = {initialize: true, index: i};
            escaped_column_name = js_string_escape_column_name(column_name, "'");
            dst_variables_map[`${prefix}['${escaped_column_name}']`] = {initialize: false, index: i};
            escaped_column_name = js_string_escape_column_name(column_name, "`");
            dst_variables_map[`${prefix}[\`${escaped_column_name}\`]`] = {initialize: false, index: i};
        }
    }
}


function parse_attribute_variables(query_text, prefix, column_names, column_names_source, dst_variables_map) {
    // The purpose of this algorithm is to minimize number of variables in varibale_map to improve performance, ideally it should be only variables from the query

    assert(prefix === 'a' || prefix === 'b');
    let rgx = new RegExp(`(?:^|[^_a-zA-Z0-9])${prefix}\\.([_a-zA-Z][_a-zA-Z0-9]*)`, 'g');
    let matches = get_all_matches(rgx, query_text);
    let column_names_from_query = matches.map(v => v[1]);
    for (let column_name of column_names_from_query) {
        let zero_based_idx = column_names.indexOf(column_name);
        if (zero_based_idx != -1) {
            dst_variables_map[`${prefix}.${column_name}`] = {initialize: true, index: zero_based_idx};
        } else {
            throw new RbqlParsingError(`Unable to find column "${column_name}" in ${prefix == 'a' ? 'input' : 'join'} ${column_names_source}`);
        }
    }
}


function map_variables_directly(query_text, column_names, dst_variables_map) {
    for (let i = 0; i < column_names.length; i++) {
        let column_name = column_names[i];
        if ( /^[_a-zA-Z][_a-zA-Z0-9]*$/.exec(column_name) === null)
            throw new RbqlIOHandlingError(`Unable to use column name "${column_name}" as RBQL/JS variable`);
        if (query_text.indexOf(column_name) != -1)
            dst_variables_map[column_name] = {initialize: true, index: i};
    }
}


function ensure_no_ambiguous_variables(query_text, input_column_names, join_column_names) {
    let join_column_names_set = new Set(join_column_names);
    for (let column_name of input_column_names) {
        if (join_column_names_set.has(column_name) && query_text.indexOf(column_name) != -1) // False positive is tolerable here
            throw new RbqlParsingError(get_ambiguous_error_msg(column_name));
    }
}


function parse_join_expression(src) {
    src = str_strip(src);
    const invalid_join_syntax_error = 'Invalid join syntax. Valid syntax: <JOIN> /path/to/B/table on a... == b... [and a... == b... [and ... ]]';
    let rgx = /^ *([^ ]+) +on +/i;
    let match = rgx.exec(src);
    if (match === null)
        throw new RbqlParsingError(invalid_join_syntax_error);
    let table_id = match[1];
    src = src.substr(match[0].length);

    let variable_pairs = [];
    var pair_rgx = /^([^ =]+) *==? *([^ =]+)/;
    var and_rgx = /^ +(and|&&) +/i;
    while (true) {
        match = pair_rgx.exec(src);
        if (match === null)
            throw new RbqlParsingError(invalid_join_syntax_error);
        variable_pairs.push([match[1], match[2]]);
        src = src.substr(match[0].length);
        if (!src.length)
            break;
        match = and_rgx.exec(src);
        if (match === null)
            throw new RbqlParsingError(invalid_join_syntax_error);
        src = src.substr(match[0].length);
    }
    return [table_id, variable_pairs];
}


function resolve_join_variables(input_variables_map, join_variables_map, variable_pairs, string_literals) {
    let lhs_variables = [];
    let rhs_indices = [];
    const valid_join_syntax_msg = 'Valid JOIN syntax: <JOIN> /path/to/B/table on a... == b... [and a... == b... [and ... ]]';
    for (let variable_pair of variable_pairs) {
        let [join_var_1, join_var_2] = variable_pair;
        join_var_1 = combine_string_literals(join_var_1, string_literals);
        join_var_2 = combine_string_literals(join_var_2, string_literals);
        if (input_variables_map.hasOwnProperty(join_var_1) && join_variables_map.hasOwnProperty(join_var_1))
            throw new RbqlParsingError(get_ambiguous_error_msg(join_var_1));
        if (input_variables_map.hasOwnProperty(join_var_2) && join_variables_map.hasOwnProperty(join_var_2))
            throw new RbqlParsingError(get_ambiguous_error_msg(join_var_2));
        if (input_variables_map.hasOwnProperty(join_var_2))
            [join_var_1, join_var_2] = [join_var_2, join_var_1];

        let [lhs_key_index, rhs_key_index] = [null, null];
        if (['NR', 'a.NR', 'aNR'].indexOf(join_var_1) != -1) {
            lhs_key_index = -1;
        } else if (input_variables_map.hasOwnProperty(join_var_1)) {
            lhs_key_index = input_variables_map[join_var_1].index;
        } else {
            throw new RbqlParsingError(`Unable to parse JOIN expression: Input table does not have field "${join_var_1}"\n${valid_join_syntax_msg}`);
        }

        if (['b.NR', 'bNR'].indexOf(join_var_2) != -1) {
            rhs_key_index = -1;
        } else if (join_variables_map.hasOwnProperty(join_var_2)) {
            rhs_key_index = join_variables_map[join_var_2].index;
        } else {
            throw new RbqlParsingError(`Unable to parse JOIN expression: Join table does not have field "${join_var_2}"\n${valid_join_syntax_msg}`);
        }

        let lhs_join_var_expression = lhs_key_index == -1 ? 'NR' : `safe_join_get(record_a, ${lhs_key_index})`;
        rhs_indices.push(rhs_key_index);
        lhs_variables.push(lhs_join_var_expression);
    }
    return [lhs_variables, rhs_indices];
}


function generate_common_init_code(query_text, variable_prefix) {
    assert(variable_prefix == 'a' || variable_prefix == 'b');
    let result = [];
    result.push(`${variable_prefix} = new Object();`);
    let base_var = variable_prefix == 'a' ? 'NR' : 'bNR';
    let attr_var = `${variable_prefix}.NR`;
    if (query_text.indexOf(attr_var) != -1)
        result.push(`${attr_var} = ${base_var};`);
    if (variable_prefix == 'a' && query_text.indexOf('aNR') != -1)
        result.push('aNR = NR;');
    return result;
}


function generate_init_statements(query_text, variables_map, join_variables_map, indent) {
    let code_lines = generate_common_init_code(query_text, 'a');
    let simple_var_name_rgx = /^[_0-9a-zA-Z]+$/;
    for (const [variable_name, var_info] of Object.entries(variables_map)) {
        if (var_info.initialize) {
            let variable_declaration_keyword = simple_var_name_rgx.exec(variable_name) ? 'var ' : '';
            code_lines.push(`${variable_declaration_keyword}${variable_name} = safe_get(record_a, ${var_info.index});`);
        }
    }
    if (join_variables_map) {
        code_lines = code_lines.concat(generate_common_init_code(query_text, 'b'));
        for (const [variable_name, var_info] of Object.entries(join_variables_map)) {
            if (var_info.initialize) {
                let variable_declaration_keyword = simple_var_name_rgx.exec(variable_name) ? 'var ' : '';
                code_lines.push(`${variable_declaration_keyword}${variable_name} = record_b === null ? null : safe_get(record_b, ${var_info.index});`);
            }
        }
    }
    for (let i = 1; i < code_lines.length; i++) {
        code_lines[i] = indent + code_lines[i];
    }
    return code_lines.join('\n');
}


function replace_star_count(aggregate_expression) {
    var rgx = /(?:(?<=^)|(?<=,)) *COUNT\( *\* *\)/ig;
    var result = aggregate_expression.replace(rgx, ' COUNT(1)');
    return str_strip(result);
}


function replace_star_vars(rbql_expression) {
    let star_rgx = /(?:^|,) *(\*|a\.\*|b\.\*) *(?=$|,)/g;
    let matches = get_all_matches(star_rgx, rbql_expression);
    let last_pos = 0;
    let result = '';
    for (let match of matches) {
        let star_expression = match[1];
        let replacement_expression = ']).concat(' + {'*': 'star_fields', 'a.*': 'record_a', 'b.*': 'record_b'}[star_expression] + ').concat([';
        if (last_pos < match.index)
            result += rbql_expression.substring(last_pos, match.index);
        result += replacement_expression;
        last_pos = match.index + match[0].length + 1; // Adding one to skip the lookahead comma
    }
    result += rbql_expression.substring(last_pos);
    return result;
}


function replace_star_vars_for_header_parsing(rbql_expression) {
    let star_rgx = /(?:(?<=^)|(?<=,)) *(\*|a\.\*|b\.\*) *(?=$|,)/g;
    let matches = get_all_matches(star_rgx, rbql_expression);
    let last_pos = 0;
    let result = '';
    for (let match of matches) {
        let star_expression = match[1];
        let replacement_expression = {'*': '__RBQL_INTERNAL_STAR', 'a.*': 'a.__RBQL_INTERNAL_STAR', 'b.*': 'b.__RBQL_INTERNAL_STAR'}[star_expression];
        if (last_pos < match.index)
            result += rbql_expression.substring(last_pos, match.index);
        result += replacement_expression;
        last_pos = match.index + match[0].length;
    }
    result += rbql_expression.substring(last_pos);
    return result;
}


function translate_update_expression(update_expression, input_variables_map, string_literals, indent) {
    let first_assignment = str_strip(update_expression.split('=')[0]);
    let first_assignment_error = `Unable to parse "UPDATE" expression: the expression must start with assignment, but "${first_assignment}" does not look like an assignable field name`;

    let assignment_looking_rgx = /(?:^|,) *(a[.#a-zA-Z0-9\[\]_]*) *=(?=[^=])/g;
    let update_expressions = [];
    let pos = 0;
    while (true) {
        let match = assignment_looking_rgx.exec(update_expression);
        if (update_expressions.length == 0 && (match === null || match.index != 0)) {
            throw new RbqlParsingError(first_assignment_error);
        }
        if (match === null) {
            update_expressions[update_expressions.length - 1] += str_strip(update_expression.substr(pos)) + ');';
            break;
        }
        if (update_expressions.length)
            update_expressions[update_expressions.length - 1] += str_strip(update_expression.substring(pos, match.index)) + ');';
        let dst_var_name = combine_string_literals(str_strip(match[1]), string_literals);
        if (!input_variables_map.hasOwnProperty(dst_var_name))
            throw new RbqlParsingError(`Unable to parse "UPDATE" expression: Unknown field name: "${dst_var_name}"`);
        let var_index = input_variables_map[dst_var_name].index;
        let current_indent = update_expressions.length ? indent : '';
        update_expressions.push(`${current_indent}safe_set(up_fields, ${var_index}, `);
        pos = match.index + match[0].length;
    }
    return combine_string_literals(update_expressions.join('\n'), string_literals);
}


function translate_select_expression(select_expression) {
    let as_alias_replacement_regexp = / +(AS|as) +([a-zA-Z][a-zA-Z0-9_]*) *(?=$|,)/g;
    let expression_without_counting_stars = replace_star_count(select_expression);
    let expression_without_as_column_alias = expression_without_counting_stars.replace(as_alias_replacement_regexp, '');
    let translated = str_strip(replace_star_vars(expression_without_as_column_alias));
    let translated_for_header = str_strip(replace_star_vars_for_header_parsing(expression_without_counting_stars));
    if (!translated.length)
        throw new RbqlParsingError('"SELECT" expression is empty');
    return [`[].concat([${translated}])`, translated_for_header];
}


function separate_string_literals(rbql_expression) {
    // The regex consists of 3 almost identicall parts, the only difference is quote type
    var rgx = /('(\\(\\\\)*'|[^'])*')|("(\\(\\\\)*"|[^"])*")|(`(\\(\\\\)*`|[^`])*`)/g;
    var match_obj = null;
    var format_parts = [];
    var string_literals = [];
    var idx_before = 0;
    while((match_obj = rgx.exec(rbql_expression)) !== null) {
        var literal_id = string_literals.length;
        var string_literal = match_obj[0];
        string_literals.push(string_literal);
        var start_index = match_obj.index;
        format_parts.push(rbql_expression.substring(idx_before, start_index));
        format_parts.push(`___RBQL_STRING_LITERAL${literal_id}___`);
        idx_before = rgx.lastIndex;
    }
    format_parts.push(rbql_expression.substring(idx_before));
    var format_expression = format_parts.join('');
    format_expression = format_expression.replace(/\t/g, ' ');
    return [format_expression, string_literals];
}


function locate_statements(rbql_expression) {
    let statement_groups = [];
    statement_groups.push([STRICT_LEFT_JOIN, LEFT_OUTER_JOIN, LEFT_JOIN, INNER_JOIN, JOIN]);
    statement_groups.push([SELECT]);
    statement_groups.push([ORDER_BY]);
    statement_groups.push([WHERE]);
    statement_groups.push([UPDATE]);
    statement_groups.push([GROUP_BY]);
    statement_groups.push([LIMIT]);
    statement_groups.push([EXCEPT]);
    var result = [];
    for (var ig = 0; ig < statement_groups.length; ig++) {
        for (var is = 0; is < statement_groups[ig].length; is++) {
            var statement = statement_groups[ig][is];
            var rgxp = new RegExp('(?:^| )' + replace_all(statement, ' ', ' *') + '(?= )', 'ig');
            var matches = get_all_matches(rgxp, rbql_expression);
            if (!matches.length)
                continue;
            if (matches.length > 1)
                throw new RbqlParsingError(`More than one "${statement}" statements found`);
            assert(matches.length == 1);
            var match = matches[0];
            var match_str = match[0];
            result.push([match.index, match.index + match_str.length, statement]);
            break; // Break to avoid matching a sub-statement from the same group e.g. "INNER JOIN" -> "JOIN"
        }
    }
    result.sort(function(a, b) { return a[0] - b[0]; });
    return result;
}


function separate_actions(rbql_expression) {
    rbql_expression = str_strip(rbql_expression);
    var result = {};
    let with_match = /^(.*)  *[Ww][Ii][Tt][Hh] *\(([a-z]{4,20})\) *$/.exec(rbql_expression);
    if (with_match !== null) {
        rbql_expression = with_match[1];
        result[WITH] = with_match[2];
    }
    var ordered_statements = locate_statements(rbql_expression);
    for (var i = 0; i < ordered_statements.length; i++) {
        var statement_start = ordered_statements[i][0];
        var span_start = ordered_statements[i][1];
        var statement = ordered_statements[i][2];
        var span_end = i + 1 < ordered_statements.length ? ordered_statements[i + 1][0] : rbql_expression.length;
        assert(statement_start < span_start);
        assert(span_start <= span_end);
        var span = rbql_expression.substring(span_start, span_end);
        var statement_params = {};
        if ([STRICT_LEFT_JOIN, LEFT_OUTER_JOIN, LEFT_JOIN, INNER_JOIN, JOIN].indexOf(statement) != -1) {
            statement_params['join_subtype'] = statement;
            statement = JOIN;
        }

        if (statement == UPDATE) {
            if (statement_start != 0)
                throw new RbqlParsingError('UPDATE keyword must be at the beginning of the query');
            span = span.replace(/^ *SET/i, '');
        }

        if (statement == ORDER_BY) {
            span = span.replace(/ ASC *$/i, '');
            var new_span = span.replace(/ DESC *$/i, '');
            if (new_span != span) {
                span = new_span;
                statement_params['reverse'] = true;
            } else {
                statement_params['reverse'] = false;
            }
        }

        if (statement == SELECT) {
            if (statement_start != 0)
                throw new RbqlParsingError('SELECT keyword must be at the beginning of the query');
            let match = /^ *TOP *([0-9]+) /i.exec(span);
            if (match !== null) {
                statement_params['top'] = parseInt(match[1]);
                span = span.substr(match.index + match[0].length);
            }
            match = /^ *DISTINCT *(COUNT)? /i.exec(span);
            if (match !== null) {
                statement_params['distinct'] = true;
                if (match[1]) {
                    statement_params['distinct_count'] = true;
                }
                span = span.substr(match.index + match[0].length);
            }
        }
        statement_params['text'] = str_strip(span);
        result[statement] = statement_params;
    }
    if (!result.hasOwnProperty(SELECT) && !result.hasOwnProperty(UPDATE)) {
        throw new RbqlParsingError('Query must contain either SELECT or UPDATE statement');
    }
    assert(result.hasOwnProperty(SELECT) != result.hasOwnProperty(UPDATE));
    return result;
}


function find_top(rb_actions) {
    if (rb_actions.hasOwnProperty(LIMIT)) {
        var rgx = /^[0-9]+$/;
        if (rgx.exec(rb_actions[LIMIT]['text']) === null) {
            throw new RbqlParsingError('LIMIT keyword must be followed by an integer');
        }
        var result = parseInt(rb_actions[LIMIT]['text']);
        return result;
    }
    var select_action = rb_actions[SELECT];
    if (select_action && select_action.hasOwnProperty('top')) {
        return select_action['top'];
    }
    return null;
}


function translate_except_expression(except_expression, input_variables_map, string_literals, input_header) {
    let skip_vars = except_expression.split(',');
    skip_vars = skip_vars.map(str_strip);
    let skip_indices = [];
    for (let var_name of skip_vars) {
        var_name = combine_string_literals(var_name, string_literals);
        if (!input_variables_map.hasOwnProperty(var_name))
            throw new RbqlParsingError(`Unknown field in EXCEPT expression: "${var_name}"`);
        skip_indices.push(input_variables_map[var_name].index);
    }
    skip_indices = skip_indices.sort((a, b) => a - b);
    let output_header = input_header === null ? null : select_except(input_header, skip_indices);
    let indices_str = skip_indices.join(',');
    return [output_header, `select_except(record_a, [${indices_str}])`];
}


class HashJoinMap {
    constructor(record_iterator, key_indices) {
        this.max_record_len = 0;
        this.hash_map = new Map();
        this.record_iterator = record_iterator;
        this.nr = 0;
        if (key_indices.length == 1) {
            this.key_index = key_indices[0];
            this.key_indices = null;
            this.polymorphic_get_key = this.get_single_key;
        } else {
            this.key_index = null;
            this.key_indices = key_indices;
            this.polymorphic_get_key = this.get_multi_key;
        }
    }

    get_single_key(nr, fields) {
        if (this.key_index >= fields.length)
            throw new RbqlRuntimeError(`No field with index ${this.key_index + 1} at record ${this.nr} in "B" table`);
        return this.key_index === -1 ? this.nr : fields[this.key_index];
    };

    get_multi_key(nr, fields) {
        let result = [];
        for (let ki of this.key_indices) {
            if (ki >= fields.length)
                throw new RbqlRuntimeError(`No field with index ${ki + 1} at record ${this.nr} in "B" table`);
            result.push(ki === -1 ? this.nr : fields[ki]);
        }
        return JSON.stringify(result);
    };

    build() {
        while (true) {
            let fields = this.record_iterator.get_record();
            if (fields === null)
                break;
            this.nr += 1;
            let nf = fields.length;
            this.max_record_len = Math.max(this.max_record_len, nf);
            let key = this.polymorphic_get_key(this.nr, fields);
            let key_records = this.hash_map.get(key);
            if (key_records === undefined) {
                this.hash_map.set(key, [[this.nr, nf, fields]]);
            } else {
                key_records.push([this.nr, nf, fields]);
            }
        }
    };

    get_join_records(key) {
        let result = this.hash_map.get(key);
        if (result === undefined)
            return [];
        return result;
    };

    get_warnings() {
        return this.record_iterator.get_warnings();
    };
}


function cleanup_query(query_text) {
    return query_text.split('\n').map(strip_comments).filter(line => line.length).join(' ').replace(/;+$/g, '');
}


function remove_redundant_table_name(query_text) {
    query_text = str_strip(query_text.replace(/ +from +a(?: +|$)/gi, ' '));
    query_text = str_strip(query_text.replace(/^ *update +a +set /gi, 'update '));
    return query_text;
}


function select_output_header(input_header, join_header, query_column_infos) {
    if (input_header === null) {
        assert(join_header === null);
    }
    let query_has_star = false;
    let query_has_column_alias = false;
    for (let qci of query_column_infos) {
        query_has_star = query_has_star || (qci !== null && qci.is_star);
        query_has_column_alias = query_has_column_alias || (qci !== null && qci.alias_name !== null);
    }
    if (input_header === null) {
        if (query_has_star && query_has_column_alias) {
            throw new RbqlParsingError('Using both * (star) and AS alias in the same query is not allowed for input tables without header');
        }
        if (!query_has_column_alias) {
            // Input table has no header and query has no aliases therefore the output table will be without header.
            return null;
        }
        input_header = [];
        join_header = [];
    }
    if (join_header === null) {
        // This means there is no JOIN table.
        join_header = [];
    }
    let output_header = [];
    for (let qci of query_column_infos) {
        // TODO refactor this and python version: extract this code into a function instead to always return something
        if (qci === null) {
            output_header.push('col' + (output_header.length + 1));
        } else if (qci.is_star) {
            if (qci.table_name === null) {
                output_header = output_header.concat(input_header).concat(join_header);
            } else if (qci.table_name === 'a') {
                output_header = output_header.concat(input_header);
            } else if (qci.table_name === 'b') {
                output_header = output_header.concat(join_header);
            }
        } else if (qci.column_name !== null) {
            output_header.push(qci.column_name);
        } else if (qci.alias_name !== null) {
            output_header.push(qci.alias_name);
        } else if (qci.column_index !== null) {
            if (qci.table_name == 'a' && qci.column_index < input_header.length) {
                output_header.push(input_header[qci.column_index]);
            } else if (qci.table_name == 'b' && qci.column_index < join_header.length) {
                output_header.push(join_header[qci.column_index]);
            } else {
                output_header.push('col' + (output_header.length + 1));
            }
        } else { // Should never happen
            output_header.push('col' + (output_header.length + 1));
        }
    }
    return output_header;
}


function sample_first_two_inconsistent_records(inconsistent_records_info) {
    let entries = Array.from(inconsistent_records_info.entries());
    entries.sort(function(a, b) { return a[1] - b[1]; });
    assert(entries.length > 1);
    let [num_fields_1, record_num_1] = entries[0];
    let [num_fields_2, record_num_2] = entries[1];
    return [record_num_1, num_fields_1, record_num_2, num_fields_2];
}


function make_inconsistent_num_fields_warning(table_name, inconsistent_records_info) {
    let [record_num_1, num_fields_1, record_num_2, num_fields_2] = sample_first_two_inconsistent_records(inconsistent_records_info);
    let warn_msg = `Number of fields in "${table_name}" table is not consistent: `;
    warn_msg += `e.g. record ${record_num_1} -> ${num_fields_1} fields, record ${record_num_2} -> ${num_fields_2} fields`;
    return warn_msg;
}


class RBQLInputIterator {
    constructor(){}
    stop() {
        throw new Error("Unable to call the interface method");
    }
    get_variables_map(query_text) {
        throw new Error("Unable to call the interface method");
    }
    get_record() {
        throw new Error("Unable to call the interface method");
    }
    handle_query_modifier() {
        return; // Reimplement if you need to handle a boolean query modifier that can be used like this: `SELECT * WITH (modifiername)`
    }
    get_warnings() {
        return []; // Reimplement if your class can produce warnings
    }
    get_header() {
        return null; // Reimplement if your class can provide input header
    }
}


class RBQLOutputWriter {
    constructor(){}

    write(fields) {
        throw new Error("Unable to call the interface method");
    }

    finish() {
        // Reimplement if your class needs to do something on finish e.g. cleanup
    };

    get_warnings() {
        return []; // Reimplement if your class can produce warnings
    };

    set_header() {
        return; // Reimplement if your class can handle output headers in a meaningful way
    }
}


class RBQLTableRegistry {
    constructor(){}

    get_iterator_by_table_id(table_id) {
        throw new Error("Unable to call the interface method");
    }

    get_warnings() {
        return []; // Reimplement if your class can produce warnings
    };
}


class TableIterator extends RBQLInputIterator {
    constructor(table, column_names=null, normalize_column_names=true, variable_prefix='a') {
        super();
        this.table = table;
        this.column_names = column_names;
        this.normalize_column_names = normalize_column_names;
        this.variable_prefix = variable_prefix;
        this.nr = 0;
        this.fields_info = new Map();
        this.stopped = false;
    }


    stop() {
        this.stopped = true;
    };


    get_variables_map(query_text) {
        let variable_map = new Object();
        parse_basic_variables(query_text, this.variable_prefix, variable_map);
        parse_array_variables(query_text, this.variable_prefix, variable_map);
        if (this.column_names !== null) {
            if (this.table.length && this.column_names.length != this.table[0].length)
                throw new RbqlIOHandlingError('List of column names and table records have different lengths');
            if (this.normalize_column_names) {
                parse_dictionary_variables(query_text, this.variable_prefix, this.column_names, variable_map);
                parse_attribute_variables(query_text, this.variable_prefix, this.column_names, 'column names list', variable_map);
            } else {
                map_variables_directly(query_text, this.column_names, variable_map);
            }
        }
        return variable_map;
    };


    get_record() {
        if (this.stopped)
            return null;
        if (this.nr >= this.table.length)
            return null;
        let record = this.table[this.nr];
        this.nr += 1;
        let num_fields = record.length;
        if (!this.fields_info.has(num_fields))
            this.fields_info.set(num_fields, this.nr);
        return record;
    };

    get_warnings() {
        if (this.fields_info.size > 1)
            return [make_inconsistent_num_fields_warning('input', this.fields_info)];
        return [];
    };

    get_header() {
        return this.column_names;
    }
}


class TableWriter extends RBQLOutputWriter {
    constructor(external_table) {
        super();
        this.table = external_table;
        this.header = null;
    }

    write(fields) {
        this.table.push(fields);
        return true;
    };

    set_header(header) {
        this.header = header;
    }
}


class SingleTableRegistry extends RBQLTableRegistry {
    constructor(table, column_names=null, normalize_column_names=true, table_id='b') {
        super();
        this.table = table;
        this.table_id = table_id;
        this.column_names = column_names;
        this.normalize_column_names = normalize_column_names;
    }

    get_iterator_by_table_id(table_id) {
        if (table_id.toLowerCase() !== this.table_id)
            throw new RbqlIOHandlingError(`Unable to find join table: "${table_id}"`);
        return new TableIterator(this.table, this.column_names, this.normalize_column_names, 'b');
    };
}


function shallow_parse_input_query(query_text, input_iterator, join_tables_registry, query_context) {
    query_text = cleanup_query(query_text);
    var [format_expression, string_literals] = separate_string_literals(query_text);
    format_expression = remove_redundant_table_name(format_expression);

    var rb_actions = separate_actions(format_expression);
    if (rb_actions.hasOwnProperty(WITH)) {
        input_iterator.handle_query_modifier(rb_actions[WITH]);
    }
    var input_variables_map = input_iterator.get_variables_map(query_text);

    if (rb_actions.hasOwnProperty(ORDER_BY) && rb_actions.hasOwnProperty(UPDATE))
        throw new RbqlParsingError('"ORDER BY" is not allowed in "UPDATE" queries');

    if (rb_actions.hasOwnProperty(GROUP_BY)) {
        if (rb_actions.hasOwnProperty(ORDER_BY) || rb_actions.hasOwnProperty(UPDATE))
            throw new RbqlParsingError('"ORDER BY", "UPDATE" and "DISTINCT" keywords are not allowed in aggregate queries');
        query_context.aggregation_key_expression = '[' + combine_string_literals(rb_actions[GROUP_BY]['text'], string_literals) + ']';
        query_context.aggregation_stage = 1;
    }


    let input_header = input_iterator.get_header();
    let join_variables_map = null;
    let join_header = null;
    if (rb_actions.hasOwnProperty(JOIN)) {
        var [rhs_table_id, variable_pairs] = parse_join_expression(rb_actions[JOIN]['text']);
        if (join_tables_registry === null)
            throw new RbqlParsingError('JOIN operations are not supported by the application');
        let join_record_iterator = join_tables_registry.get_iterator_by_table_id(rhs_table_id);
        if (!join_record_iterator)
            throw new RbqlParsingError(`Unable to find join table: "${rhs_table_id}"`);
        if (rb_actions.hasOwnProperty(WITH)) {
            join_record_iterator.handle_query_modifier(rb_actions[WITH]);
        }
        join_variables_map = join_record_iterator.get_variables_map(query_text);
        join_header = join_record_iterator.get_header();
        if (input_header === null && join_header !== null) {
            throw new RbqlIOHandlingError('Inconsistent modes: Input table doesn\'t have a header while the Join table has a header');
        }
        if (input_header !== null && join_header === null) {
            throw new RbqlIOHandlingError('Inconsistent modes: Input table has a header while the Join table doesn\'t have a header');
        }
        let [lhs_variables, rhs_indices] = resolve_join_variables(input_variables_map, join_variables_map, variable_pairs, string_literals);
        let sql_join_type = {'JOIN': InnerJoiner, 'INNER JOIN': InnerJoiner, 'LEFT JOIN': LeftJoiner, 'LEFT OUTER JOIN': LeftJoiner, 'STRICT LEFT JOIN': StrictLeftJoiner}[rb_actions[JOIN]['join_subtype']];
        query_context.lhs_join_var_expression = lhs_variables.length == 1 ? lhs_variables[0] : 'JSON.stringify([' + lhs_variables.join(',') + '])';
        query_context.join_map_impl = new HashJoinMap(join_record_iterator, rhs_indices);
        query_context.join_map_impl.build();
        query_context.join_map = new sql_join_type(query_context.join_map_impl);
    }

    query_context.variables_init_code = combine_string_literals(generate_init_statements(format_expression, input_variables_map, join_variables_map, ' '.repeat(4)), string_literals);

    if (rb_actions.hasOwnProperty(WHERE)) {
        var where_expression = rb_actions[WHERE]['text'];
        if (/[^><!=]=[^=]/.exec(where_expression))
            throw new RbqlParsingError('Assignments "=" are not allowed in "WHERE" expressions. For equality test use "==" or "==="');
        query_context.where_expression = combine_string_literals(where_expression, string_literals);
    }

    if (rb_actions.hasOwnProperty(UPDATE)) {
        var update_expression = translate_update_expression(rb_actions[UPDATE]['text'], input_variables_map, string_literals, ' '.repeat(8));
        query_context.update_expressions = combine_string_literals(update_expression, string_literals);
        query_context.writer.set_header(input_header);
    }

    if (rb_actions.hasOwnProperty(SELECT)) {
        query_context.top_count = find_top(rb_actions);
        if (rb_actions.hasOwnProperty(EXCEPT)) {
            if (rb_actions.hasOwnProperty(JOIN)) {
                throw new RbqlParsingError('EXCEPT and JOIN are not allowed in the same query');
            }
            let [output_header, select_expression] = translate_except_expression(rb_actions[EXCEPT]['text'], input_variables_map, string_literals, input_header);
            query_context.select_expression = select_expression;
            query_context.writer.set_header(output_header);
        } else {
            let [select_expression, select_expression_for_header] = translate_select_expression(rb_actions[SELECT]['text']);
            query_context.select_expression = combine_string_literals(select_expression, string_literals);
            let column_infos = adhoc_parse_select_expression_to_column_infos(select_expression_for_header, string_literals);
            let output_header = select_output_header(input_header, join_header, column_infos);
            query_context.writer.set_header(output_header);
        }

        query_context.writer = new TopWriter(query_context.writer, query_context.top_count);
        if (rb_actions[SELECT].hasOwnProperty('distinct_count')) {
            query_context.writer = new UniqCountWriter(query_context.writer);
        } else if (rb_actions[SELECT].hasOwnProperty('distinct')) {
            query_context.writer = new UniqWriter(query_context.writer);
        }
    }

    if (rb_actions.hasOwnProperty(ORDER_BY)) {
        query_context.sort_key_expression = combine_string_literals(rb_actions[ORDER_BY]['text'], string_literals);
        let reverse_sort = rb_actions[ORDER_BY]['reverse'];
        query_context.writer = new SortedWriter(query_context.writer, reverse_sort);
    }
}


function query(query_text, input_iterator, output_writer, output_warnings, join_tables_registry=null, user_init_code='') {
    query_context = new RBQLContext(query_text, input_iterator, output_writer, user_init_code);
    shallow_parse_input_query(query_text, input_iterator, join_tables_registry, query_context);
    compile_and_run(query_context);
    query_context.writer.finish();
    output_warnings.push(...input_iterator.get_warnings());
    if (query_context.join_map_impl)
        output_warnings.push(...query_context.join_map_impl.get_warnings());
    output_warnings.push(...output_writer.get_warnings());
}


function query_table(query_text, input_table, output_table, output_warnings, join_table=null, input_column_names=null, join_column_names=null, output_column_names=null, normalize_column_names=true, user_init_code='') {
    if (!normalize_column_names && input_column_names !== null && join_column_names !== null)
        ensure_no_ambiguous_variables(query_text, input_column_names, join_column_names);
    let input_iterator = new TableIterator(input_table, input_column_names, normalize_column_names);
    let output_writer = new TableWriter(output_table);
    let join_tables_registry = join_table === null ? null : new SingleTableRegistry(join_table, join_column_names, normalize_column_names);
    query(query_text, input_iterator, output_writer, output_warnings, join_tables_registry, user_init_code);
    if (output_column_names !== null) {
        assert(output_column_names.length == 0, '`output_column_names` param must be an empty list or null');
        if (output_writer.header !== null) {
            for (let column_name of output_writer.header) {
                output_column_names.push(column_name);
            }
        }
    }
}


function exception_to_error_info(e) {
    let exceptions_type_map = {
        'RbqlRuntimeError': 'query execution',
        'RbqlParsingError': 'query parsing',
        'SyntaxError': 'JS syntax error',
        'RbqlIOHandlingError': 'IO handling'
    };
    let error_type = 'unexpected';
    if (e.constructor && e.constructor.name && exceptions_type_map.hasOwnProperty(e.constructor.name)) {
        error_type = exceptions_type_map[e.constructor.name];
    }
    let error_msg = e.hasOwnProperty('message') ? e.message : String(e);
    return [error_type, error_msg];
}


exports.query = query;
exports.query_table = query_table;
exports.RBQLInputIterator = RBQLInputIterator;
exports.RBQLOutputWriter = RBQLOutputWriter;
exports.RBQLTableRegistry = RBQLTableRegistry;

exports.version = RBQL_VERSION;
exports.TableIterator = TableIterator;
exports.TableWriter = TableWriter;
exports.SingleTableRegistry = SingleTableRegistry;
exports.exception_to_error_info = exception_to_error_info;


// The functions below are exported just for unit tests, they are not part of the rbql API
// TODO exports through the special unit_test proxy e.g. exports.unit_test.parse_basic_variables = parse_basic_variables;
exports.parse_basic_variables = parse_basic_variables;
exports.parse_array_variables = parse_array_variables;
exports.parse_dictionary_variables = parse_dictionary_variables;
exports.parse_attribute_variables = parse_attribute_variables;
exports.get_all_matches = get_all_matches;
exports.strip_comments = strip_comments;
exports.separate_actions = separate_actions;
exports.separate_string_literals = separate_string_literals;
exports.combine_string_literals = combine_string_literals;
exports.parse_join_expression = parse_join_expression;
exports.resolve_join_variables = resolve_join_variables;
exports.translate_update_expression = translate_update_expression;
exports.translate_select_expression = translate_select_expression;
exports.translate_except_expression = translate_except_expression;
exports.like_to_regex = like_to_regex;
exports.adhoc_parse_select_expression_to_column_infos = adhoc_parse_select_expression_to_column_infos;
exports.replace_star_count = replace_star_count;
exports.replace_star_vars_for_header_parsing = replace_star_vars_for_header_parsing;
exports.select_output_header = select_output_header;
exports.sample_first_two_inconsistent_records = sample_first_two_inconsistent_records;

}(typeof exports === 'undefined' ? this.rbql = {} : exports));


