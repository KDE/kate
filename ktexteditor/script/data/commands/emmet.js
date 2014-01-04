/* kate-script
* author: Gregor Petrin
* license: BSD
* revision: 1
* kate-version: 3.4
* functions: emmetExpand, emmetWrap, emmetSelectTagPairInwards, emmetSelectTagPairOutwards, emmetMatchingPair, emmetToggleComment ,emmetNext, emmetPrev, emmetSelectNext, emmetSelectPrev, emmetDelete, emmetSplitJoinTab, emmetEvaluateMathExpression, emmetDecrementNumberBy1, emmetDecrementNumberBy10, emmetDecrementNumberBy01, emmetIncrementNumberBy1, emmetIncrementNumberBy10, emmetIncrementNumberBy01
*
*/

// required katepart js libraries
require ("range.js");
require ("emmet/lib.js");
require ("emmet/editor_interface.js");

function help(cmd)
{
	cmds = {
		"emmetExpand" : i18n("Expands the abbreviation using Emmet expressions; see http://code.google.com/p/zen-coding/wiki/ZenHTMLSelectorsEn"),
		"emmetWrap" : i18n("Wraps the selected text in XML tags constructed from the provided Emmet expression (defaults to div)."),
		"emmetMatchingPair" : i18n("Moves the caret to the current tag's pair"),
		"emmetSelectTagPairInwards" : i18n("Select contents of HTML/XML tag, moving inward on continuous invocations"),
		"emmetSelectTagPairOutwards" : i18n("Select contents of HTML/XML tag, moving outwards on continuous invocations"),
		"emmetNext" : i18n("Move to the next edit point (tag or empty attribute)."),
		"emmetPrev" : i18n("Move to the previous edit point (tag or empty attribute)."),
		"emmetSelectNext" : i18n("Select next edit point (tag or empty attribute)."),
		"emmetSelectPrev" : i18n("Select previous edit point (tag or empty attribute)."),
		"emmetToggleComment" : i18n("Toggle comment of current tag or CSS selector"),
		"emmetDelete" : i18n("Deletes tag under cursor"),
		"emmetSplitJoinTab" : i18n("Splits or joins a tag"),
		"emmetEvaluateMathExpression" : i18n("Evaluates a simple math expression"),
		"emmetDecrementNumberBy1" : i18n("Decrement number under cursor by 1"),
		"emmetDecrementNumberBy10" : i18n("Decrement number under cursor by 10"),
		"emmetDecrementNumberBy01" : i18n("Decrement number under cursor by 0.1"),
		"emmetIncrementNumberBy1" : i18n("Increment number under cursor by 1"),
		"emmetIncrementNumberBy10" : i18n("Increment number under cursor by 10"),
		"emmetIncrementNumberBy01" : i18n("Increment number under cursor by 0.1")
	}
	return cmds[cmd];
}

function action(cmd)
{
		var a = new Object();
		a.icon = "";
		a.category = i18n("Emmet");
		a.shortcut = "";
		a.interactive = false;
		if (cmd == "emmetExpand") {
			a.text = i18n("Expand abbreviation");
		} else if (cmd == "emmetWrap") {
			a.text = i18n("Wrap with tag");
			a.interactive = true;		
		} else if (cmd == "emmetMatchingPair") {
			a.text = i18n("Move cursor to matching tag");
		} else if (cmd == "emmetSelectTagPairInwards") {
			a.text = i18n("Select HTML/XML tag contents inwards");
		} else if (cmd == "emmetSelectTagPairOutwards") {
			a.text = i18n("Select HTML/XML tag contents outwards");
		} else if (cmd == "emmetToggleComment") {
			a.text = i18n("Toggle comment"); // FIXME Action w/ that name already exists @ part/view/kateview.cpp:411
		} else if (cmd == "emmetNext") {
			a.text = i18n("Go to next edit point");
		} else if (cmd == "emmetPrev") {
			a.text = i18n("Go to previous edit point");
		} else if (cmd == "emmetSelectNext") {
			a.text = i18n("Select next edit point");
		} else if (cmd == "emmetSelectPrev") {
			a.text = i18n("Select previous edit point");
		} else if (cmd == "emmetDelete") {
			a.text = i18n("Delete tag under cursor");
		} else if (cmd == "emmetSplitJoinTab") {
			a.text = i18n("Split or join a tag");
		} else if (cmd == "emmetEvaluateMathExpression") {
			a.text = i18n("Evaluate a simple math expression");
		} else if (cmd == "emmetDecrementNumberBy1") {
			a.text = i18n("Decrement number by 1");
		} else if (cmd == "emmetDecrementNumberBy10") {
			a.text = i18n("Decrement number by 10");
		} else if (cmd == "emmetDecrementNumberBy01") {
			a.text = i18n("Decrement number by 0.1");
		} else if (cmd == "emmetIncrementNumberBy1") {
			a.text = i18n("Increment number by 1");
		} else if (cmd == "emmetIncrementNumberBy10") {
			a.text = i18n("Increment number by 10");
		} else if (cmd == "emmetIncrementNumberBy01") {
			a.text = i18n("Increment number by 0.1");
		}
		return a;
}

//Returns the kate editor interface
function getInterface() {
	var kateInterface = new zen_editor(document, view);
	return kateInterface;
}

function emmetExpand() {
	emmet.require('actions').run('expand_abbreviation', getInterface());
}   

function emmetWrap(par) {
	emmet.require('actions').run('wrap_with_abbreviation', getInterface(), par || 'div');
}

function emmetMatchingPair() {
	emmet.require('actions').run('matching_pair', getInterface());
}

function emmetSelectTagPairInwards() {
	emmet.require('actions').run('match_pair', getInterface());
}

function emmetSelectTagPairOutwards() {
	emmet.require('actions').run('match_pair', getInterface());
}

function emmetNext() {
	emmet.require('actions').run('next_edit_point', getInterface());
}

function emmetPrev() {
	emmet.require('actions').run('prev_edit_point', getInterface());
}

function emmetSelectNext() {
	emmet.require('actions').run('select_next_item', getInterface());
}

function emmetSelectPrev() {
	emmet.require('actions').run('select_previous_item', getInterface());
}

function emmetToggleComment() {
	emmet.require('actions').run('toggle_comment', getInterface());
}

function emmetDelete() {
	emmet.require('actions').run('remove_tag', getInterface());
}

function emmetSplitJoinTab() {
	emmet.require('actions').run('split_join_tag', getInterface());
}

function emmetEvaluateMathExpression() {
	emmet.require('actions').run('evaluate_math_expression', getInterface());
}

function emmetIncrementNumberBy1() {
	emmet.require('actions').run('increment_number_by_1', getInterface());
}

function emmetIncrementNumberBy10() {
	emmet.require('actions').run('increment_number_by_10', getInterface());
}

function emmetIncrementNumberBy01() {
	emmet.require('actions').run('increment_number_by_01', getInterface());
}

function emmetDecrementNumberBy1() {
	emmet.require('actions').run('decrement_number_by_1', getInterface());
}

function emmetDecrementNumberBy10() {
	emmet.require('actions').run('decrement_number_by_10', getInterface());
}

function emmetDecrementNumberBy01() {
	emmet.require('actions').run('decrement_number_by_01', getInterface());
}
