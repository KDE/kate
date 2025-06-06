/*
    SPDX-FileCopyrightText: 2018 Sven Brauch <mail@svenbrauch.de>
    SPDX-FileCopyrightText: 2018 Michal Srb <michalsrb@gmail.com>
    SPDX-FileCopyrightText: 2020 Jan Paul Batrina <jpmbatrina01@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "colorpickerconfigpage.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QGroupBox>
#include <QVBoxLayout>

KTextEditor::ConfigPage *KateColorPickerPlugin::configPage(int number, QWidget *parent)
{
    if (number != 0) {
        return nullptr;
    }

    return new KateColorPickerConfigPage(parent, this);
}

KateColorPickerConfigPage::KateColorPickerConfigPage(QWidget *parent, KateColorPickerPlugin *plugin)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    chkNamedColors = new QCheckBox(i18n("Show preview for known color names"), this);
    chkNamedColors->setToolTip(i18n(
        "Also show the color picker for known color names (e.g. skyblue).\nSee https://www.w3.org/TR/SVG11/types.html#ColorKeywords for the list of colors."));
    layout->addWidget(chkNamedColors);

    chkPreviewAfterColor = new QCheckBox(i18n("Place preview after text color"), this);
    layout->addWidget(chkPreviewAfterColor);

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(chkNamedColors, &QCheckBox::stateChanged, this, &KateColorPickerConfigPage::changed);
#else
    connect(chkNamedColors, &QCheckBox::checkStateChanged, this, &KateColorPickerConfigPage::changed);
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(chkPreviewAfterColor, &QCheckBox::stateChanged, this, &KateColorPickerConfigPage::changed);
#else
    connect(chkPreviewAfterColor, &QCheckBox::checkStateChanged, this, &KateColorPickerConfigPage::changed);
#endif

    auto *hexGroup = new QGroupBox(i18n("Hex color matching"), this);
    auto *hexLayout = new QVBoxLayout();
    // Hex color formats supported by QColor. See https://doc.qt.io/qt-5/qcolor.html#setNamedColor
    chkHexLengths.insert_or_assign(12, new QCheckBox(i18n("12 digits (#RRRRGGGGBBBB)"), this));
    chkHexLengths.insert_or_assign(9, new QCheckBox(i18n("9 digits (#RRRGGGBBB)"), this));
    chkHexLengths.insert_or_assign(8, new QCheckBox(i18n("8 digits (#AARRGGBB)"), this));
    chkHexLengths.insert_or_assign(6, new QCheckBox(i18n("6 digits (#RRGGBB)"), this));
    chkHexLengths.insert_or_assign(3, new QCheckBox(i18n("3 digits (#RGB)"), this));

    for (const auto &[_, chk] : std::as_const(chkHexLengths)) {
        hexLayout->addWidget(chk);
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
        connect(chk, &QCheckBox::stateChanged, this, &KateColorPickerConfigPage::changed);
#else
        connect(chk, &QCheckBox::checkStateChanged, this, &KateColorPickerConfigPage::changed);
#endif
    }
    hexGroup->setLayout(hexLayout);
    layout->addWidget(hexGroup);

    layout->addStretch();

    connect(this, &KateColorPickerConfigPage::changed, this, [this]() {
        m_colorConfigChanged = true;
    });

    reset();
}

QString KateColorPickerConfigPage::name() const
{
    return i18n("Color Picker");
}

QString KateColorPickerConfigPage::fullName() const
{
    return i18n("Color Picker Settings");
}

QIcon KateColorPickerConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("color-picker"));
}

void KateColorPickerConfigPage::apply()
{
    if (!m_colorConfigChanged) {
        // apply() gets called when the "Apply" or "OK" button is pressed. This means that if a user presses "Apply" THEN "OK", the config is updated twice
        // Since the the regeneration of color note positions is expensive, we only update on the first call to apply() before changes are made again
        return;
    }

    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ColorPicker"));
    config.writeEntry("NamedColors", chkNamedColors->isChecked());
    config.writeEntry("PreviewAfterColor", chkPreviewAfterColor->isChecked());

    QList<int> hexLengths;
    for (const auto &[hexLength, chkBox] : chkHexLengths) {
        if (chkBox->isChecked()) {
            hexLengths.append(hexLength);
        }
    }
    config.writeEntry("HexLengths", hexLengths);

    config.sync();
    m_plugin->readConfig();
    m_colorConfigChanged = false;
}

void KateColorPickerConfigPage::reset()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ColorPicker"));
    chkNamedColors->setChecked(config.readEntry("NamedColors", false));
    chkPreviewAfterColor->setChecked(config.readEntry("PreviewAfterColor", true));

    const QList<int> enabledHexLengths = config.readEntry("HexLengths", QList<int>{12, 9, 6, 3});

    for (const auto &[hexLength, chkBox] : chkHexLengths) {
        chkBox->setChecked(enabledHexLengths.contains(hexLength));
    }
}
