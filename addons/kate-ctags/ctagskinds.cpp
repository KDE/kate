/***************************************************************************
 *   SPDX-FileCopyrightText: 2001-2002 Bernd Gehrmann *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 ***************************************************************************/

#include "ctagskinds.h"

#include <KLazyLocalizedString>

struct CTagsKindMapping {
    char abbrev;
    const KLazyLocalizedString verbose;
};

struct CTagsExtensionMapping {
    const char *extension;
    const CTagsKindMapping *kinds;
};

static const CTagsKindMapping kindMappingAsm[] = {{'d', kli18nc("Tag Type", "define")},
                                                  {'l', kli18nc("Tag Type", "label")},
                                                  {'m', kli18nc("Tag Type", "macro")},
                                                  {0, {}}};

static const CTagsKindMapping kindMappingAsp[] = {{'f', kli18nc("Tag Type", "function")}, {'s', kli18nc("Tag Type", "subroutine")}, {0, {}}};

static const CTagsKindMapping kindMappingAwk[] = {{'f', kli18nc("Tag Type", "function")}, {0, {}}};

static const CTagsKindMapping kindMappingBeta[] = {{'f', kli18nc("Tag Type", "fragment definition")},
                                                   {'p', kli18nc("Tag Type", "any pattern")},
                                                   {'s', kli18nc("Tag Type", "slot")},
                                                   {'v', kli18nc("Tag Type", "pattern")},
                                                   {0, {}}};

static const CTagsKindMapping kindMappingC[] = {{'c', kli18nc("Tag Type", "class")},
                                                {'d', kli18nc("Tag Type", "macro")},
                                                {'e', kli18nc("Tag Type", "enumerator")},
                                                {'f', kli18nc("Tag Type", "function")},
                                                {'g', kli18nc("Tag Type", "enumeration")},
                                                {'m', kli18nc("Tag Type", "member")},
                                                {'n', kli18nc("Tag Type", "namespace")},
                                                {'p', kli18nc("Tag Type", "prototype")},
                                                {'s', kli18nc("Tag Type", "struct")},
                                                {'t', kli18nc("Tag Type", "typedef")},
                                                {'u', kli18nc("Tag Type", "union")},
                                                {'v', kli18nc("Tag Type", "variable")},
                                                {'x', kli18nc("Tag Type", "external variable")},
                                                {0, {}}};

static const CTagsKindMapping kindMappingCobol[] = {{'p', kli18nc("Tag Type", "paragraph")}, {0, {}}};

static const CTagsKindMapping kindMappingEiffel[] = {{'c', kli18nc("Tag Type", "class")},
                                                     {'f', kli18nc("Tag Type", "feature")},
                                                     {'l', kli18nc("Tag Type", "local entity")},
                                                     {0, {}}};

static const CTagsKindMapping kindMappingFortran[] = {{'b', kli18nc("Tag Type", "block")},
                                                      {'c', kli18nc("Tag Type", "common")},
                                                      {'e', kli18nc("Tag Type", "entry")},
                                                      {'f', kli18nc("Tag Type", "function")},
                                                      {'i', kli18nc("Tag Type", "interface")},
                                                      {'k', kli18nc("Tag Type", "type component")},
                                                      {'l', kli18nc("Tag Type", "label")},
                                                      {'L', kli18nc("Tag Type", "local")},
                                                      {'m', kli18nc("Tag Type", "module")},
                                                      {'n', kli18nc("Tag Type", "namelist")},
                                                      {'p', kli18nc("Tag Type", "program")},
                                                      {'s', kli18nc("Tag Type", "subroutine")},
                                                      {'t', kli18nc("Tag Type", "type")},
                                                      {'v', kli18nc("Tag Type", "variable")},
                                                      {0, {}}};

static const CTagsKindMapping kindMappingJava[] = {{'c', kli18nc("Tag Type", "class")},
                                                   {'f', kli18nc("Tag Type", "field")},
                                                   {'i', kli18nc("Tag Type", "interface")},
                                                   {'m', kli18nc("Tag Type", "method")},
                                                   {'p', kli18nc("Tag Type", "package")},
                                                   {0, {}}};

static const CTagsKindMapping kindMappingLisp[] = {{'f', kli18nc("Tag Type", "function")}, {0, {}}};

static const CTagsKindMapping kindMappingMake[] = {{'m', kli18nc("Tag Type", "macro")}, {0, {}}};

static const CTagsKindMapping kindMappingPascal[] = {{'f', kli18nc("Tag Type", "function")}, {'p', kli18nc("Tag Type", "procedure")}, {0, {}}};

static const CTagsKindMapping kindMappingPerl[] = {{'s', kli18nc("Tag Type", "subroutine")}, {0, {}}};

static const CTagsKindMapping kindMappingPHP[] = {{'c', kli18nc("Tag Type", "class")}, {'f', kli18nc("Tag Type", "function")}, {0, {}}};

static const CTagsKindMapping kindMappingPython[] = {{'c', kli18nc("Tag Type", "class")}, {'f', kli18nc("Tag Type", "function")}, {0, {}}};

static const CTagsKindMapping kindMappingRexx[] = {{'s', kli18nc("Tag Type", "subroutine")}, {0, {}}};

static const CTagsKindMapping kindMappingRuby[] = {{'c', kli18nc("Tag Type", "class")},
                                                   {'f', kli18nc("Tag Type", "function")},
                                                   {'m', kli18nc("Tag Type", "mixin")},
                                                   {0, {}}};

static const CTagsKindMapping kindMappingScheme[] = {{'f', kli18nc("Tag Type", "function")}, {'s', kli18nc("Tag Type", "set")}, {0, {}}};

static const CTagsKindMapping kindMappingSh[] = {{'f', kli18nc("Tag Type", "function")}, {0, {}}};

static const CTagsKindMapping kindMappingSlang[] = {{'f', kli18nc("Tag Type", "function")}, {'n', kli18nc("Tag Type", "namespace")}, {0, {}}};

static const CTagsKindMapping kindMappingTcl[] = {{'p', kli18nc("Tag Type", "procedure")}, {0, {}}};

static const CTagsKindMapping kindMappingVim[] = {{'f', kli18nc("Tag Type", "function")}, {0, {}}};

static CTagsExtensionMapping extensionMapping[] = {
    {"asm", kindMappingAsm},     {"s", kindMappingAsm},         {"S", kindMappingAsm},       {"asp", kindMappingAsp},
    {"asa", kindMappingAsp},     {"awk", kindMappingAwk},       {"c++", kindMappingC},       {"cc", kindMappingC},
    {"cp", kindMappingC},        {"cpp", kindMappingC},         {"cxx", kindMappingC},       {"h", kindMappingC},
    {"h++", kindMappingC},       {"hh", kindMappingC},          {"hp", kindMappingC},        {"hpp", kindMappingC},
    {"hxx", kindMappingC},       {"beta", kindMappingBeta},     {"cob", kindMappingCobol},   {"COB", kindMappingCobol},
    {"e", kindMappingEiffel},    {"f", kindMappingFortran},     {"for", kindMappingFortran}, {"ftn", kindMappingFortran},
    {"f77", kindMappingFortran}, {"f90", kindMappingFortran},   {"f95", kindMappingFortran}, {"java", kindMappingJava},
    {"cl", kindMappingLisp},     {"clisp", kindMappingLisp},    {"el", kindMappingLisp},     {"l", kindMappingLisp},
    {"lisp", kindMappingLisp},   {"lsp", kindMappingLisp},      {"ml", kindMappingLisp},     {"mak", kindMappingMake},
    {"p", kindMappingPascal},    {"pas", kindMappingPascal},    {"pl", kindMappingPerl},     {"pm", kindMappingPerl},
    {"perl", kindMappingPerl},   {"php", kindMappingPHP},       {"php3", kindMappingPHP},    {"phtml", kindMappingPHP},
    {"py", kindMappingPython},   {"python", kindMappingPython}, {"cmd", kindMappingRexx},    {"rexx", kindMappingRexx},
    {"rx", kindMappingRexx},     {"rb", kindMappingRuby},       {"sch", kindMappingScheme},  {"scheme", kindMappingScheme},
    {"scm", kindMappingScheme},  {"sm", kindMappingScheme},     {"SCM", kindMappingScheme},  {"SM", kindMappingScheme},
    {"sh", kindMappingSh},       {"SH", kindMappingSh},         {"bsh", kindMappingSh},      {"bash", kindMappingSh},
    {"ksh", kindMappingSh},      {"zsh", kindMappingSh},        {"sl", kindMappingSlang},    {"tcl", kindMappingTcl},
    {"wish", kindMappingTcl},    {"vim", kindMappingVim},       {nullptr, nullptr}};

static const CTagsKindMapping *findKindMapping(const char *pextension)
{
    CTagsExtensionMapping *pem = extensionMapping;
    while (pem->extension != nullptr) {
        if (strcmp(pem->extension, pextension) == 0) {
            return pem->kinds;
        }
        ++pem;
    }

    return nullptr;
}

QString CTagsKinds::findKind(const char *kindChar, const QString &extension)
{
    if (kindChar == nullptr || extension.isEmpty()) {
        return QString();
    }

    const CTagsKindMapping *kindMapping = findKindMapping(extension.toLocal8Bit().constData());
    if (kindMapping) {
        const CTagsKindMapping *pkm = kindMapping;
        while (!pkm->verbose.isEmpty()) {
            if (pkm->abbrev == *kindChar) {
                return pkm->verbose.toString();
            }
            ++pkm;
        }
    }

    return QString();
}

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
QString CTagsKinds::findKindNoi18n(const char *kindChar, const QStringRef &extension)
#else
QString CTagsKinds::findKindNoi18n(const char *kindChar, const QStringView &extension)
#endif
{
    if (kindChar == nullptr || extension.isEmpty()) {
        return QString();
    }

    const CTagsKindMapping *kindMapping = findKindMapping(extension.toLocal8Bit().constData());
    if (kindMapping) {
        const CTagsKindMapping *pkm = kindMapping;
        while (!pkm->verbose.isEmpty()) {
            if (pkm->abbrev == *kindChar) {
                return pkm->verbose.toString();
            }
            ++pkm;
        }
    }

    return QString();
}
