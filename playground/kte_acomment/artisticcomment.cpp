/****************************************************************************
 **    - This file is part of the Artistic Comment KTextEditor-Plugin -    **
 **  - Copyright 2009-2010 Jonathan Schmidt-Dominé <devel@the-user.org> -  **
 **                                  ----                                  **
 **   - This program is free software; you can redistribute it and/or -    **
 **    - modify it under the terms of the GNU Library General Public -     **
 **    - License version 3, or (at your option) any later version, as -    **
 ** - published by the Free Software Foundation.                         - **
 **                                  ----                                  **
 **  - This library is distributed in the hope that it will be useful, -   **
 **   - but WITHOUT ANY WARRANTY; without even the implied warranty of -   **
 ** - MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU -  **
 **          - Library General Public License for more details. -          **
 **                                  ----                                  **
 ** - You should have received a copy of the GNU Library General Public -  **
 ** - License along with the kdelibs library; see the file COPYING.LIB. -  **
 **  - If not, write to the Free Software Foundation, Inc., 51 Franklin -  **
 **      - Street, Fifth Floor, Boston, MA 02110-1301, USA. or see -       **
 **                  - <http://www.gnu.org/licenses/>. -                   **
 ***************************************************************************/

#include "artisticcomment.h"
#include <QList>
#include <QMap>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KComponentData>

static QList<QString> explode(QString str, size_t max)
{
    QList<QString> ret;
    QString curr;
    QString currWord;
    if (!str.isEmpty() && !str.endsWith('\n')) {
        str += '\n';
    }

    for(int i = 0; i != str.size(); ++i)
    {
        if(str[i] == '\n')
        {
            if((size_t)currWord.size() + (size_t)(curr.size() == 0 ? 0 : curr.size() + 1) > max)
            {
                ret.push_back(curr);
                curr = currWord;
            }
            else
            {
                ret.push_back(curr + (curr.size() == 0 ? "" : " ") + currWord);
                curr = "";
            }
            currWord = "";
        }
        else if(str[i] == ' ')
        {
            if((size_t)currWord.size() + (size_t)(curr.size() == 0 ? 0 : curr.size() + 1) > max)
            {
                ret.push_back(curr);
                curr = currWord;
            }
            else
                curr += (curr.size() == 0 ? "" : " ") + currWord;
            currWord = "";
        }
        else
        {
            currWord += str[i];
        }
    }
    if(currWord != "")
    {
        if((size_t)currWord.size() + (size_t)(curr.size() == 0 ? 0 : curr.size() + 1) > max)
        {
            ret.push_back(curr);
            ret.push_back(currWord);
        }
        else
            ret.push_back(curr + (curr.size() == 0 ? "" : " ") + currWord);
    }
    else if(curr != "")
        ret.push_back(curr);
    return ret;
}

ArtisticComment::ArtisticComment(
                    const QString& begin, const QString& end,
                    const QString& lineBegin, const QString& lineEnd,
                    const QString& textBegin, const QString& textEnd,
                    QChar lfill, QChar rfill,
                    int minfill, int realWidth,
                    bool truncate, type_t type)
    : begin(begin), end(end),
      lineBegin(lineBegin), lineEnd(lineEnd),
      textBegin(textBegin), textEnd(textEnd),
      lfill(lfill), rfill(rfill),
      minfill(minfill), realWidth(realWidth),
      truncate(truncate), type(type)
{
}

QString ArtisticComment::apply(const QString& text)
{
    size_t textWidth = realWidth - lineBegin.size() - lineEnd.size() - textBegin.size() - textEnd.size() - minfill;
    QList<QString> expl = explode(text, textWidth);
    QString ret = begin + "\n";
    switch(type)
    {
        case LeftNoFill:
        {
            for(int i = 0; i != expl.size(); ++i)
                ret += lineBegin + textBegin + expl[i] + textEnd + lineEnd + '\n';
            break;
        }
        case Left:
        {
            for(int i = 0; i != expl.size(); ++i)
            {
                QString line = lineBegin + textBegin + expl[i] + textEnd;
                size_t max = realWidth - (size_t)lineEnd.size();
                while((size_t)line.size() < max)
                    line += rfill;
                line += lineEnd + '\n';
                ret += line;
            }
            break;
        }
        case Center:
        {
            for(int i = 0; i != expl.size(); ++i)
            {
                size_t numChars = realWidth - (size_t)lineBegin.size() - (size_t)lineEnd.size() - (size_t)textBegin.size() - (size_t)textEnd.size() - (size_t)expl[i].size();
                size_t num = (numChars + !truncate) / 2;
                QString line = lineBegin;
                for(size_t j = 0; j != num; ++j)
                    line += lfill;
                line += textBegin + expl[i] + textEnd;
                num = numChars - num;
                for(size_t j = 0; j != num; ++j)
                    line += rfill;
                line += lineEnd + '\n';
                ret += line;
            }
            break;
        }
        case Right:
        {
            for(int i = 0; i != expl.size(); ++i)
            {
                size_t num = realWidth - lineBegin.size() - lineEnd.size() - textBegin.size() - textEnd.size() - expl[i].size();
                QString line = lineBegin;
                for(size_t j = 0; j != num; ++j)
                    line += lfill;
                line += textBegin + expl[i] + textEnd + lineEnd;
                ret += line;
            }
            break;
        }
    }
    return ret + end;
}

static KSharedConfigPtr m_config(KSharedConfig::openConfig(KComponentData("artisticcomment", "", KComponentData::SkipMainComponentRegistration), "", KConfig::CascadeConfig));
static QMap<QString, ArtisticComment> m_styles;

inline static ArtisticComment::type_t toTypeEnum(const QString& str)
{
    switch (str.size())
    {
    case 10:
        return ArtisticComment::LeftNoFill;
    case 6:
        return ArtisticComment::Center;
    case 5:
        return ArtisticComment::Right;
    default:
        return ArtisticComment::Left;
    }
}

void ArtisticComment::readConfig()
{
    m_styles.clear();
    QStringList groups = m_config->groupList();
    #define ED(str, def) sub.readEntry(str, def)
    foreach(QString str, groups)
    {
        KConfigGroup sub = m_config->group(str);
        m_styles[str] = ArtisticComment(ED("begin", ""), ED("end", ""),
                                        ED("lineBegin", ""), ED("lineEnd", ""),
                                        ED("textBegin", ""), ED("textEnd", ""),
                                        ED("lfill", " ")[0], ED("rfill", " ")[0],
                                        ED("minfill", 0), ED("realWidth", 60), ED("truncate", true), toTypeEnum(ED("type", "Left")));
    }
    #undef ED
}

void ArtisticComment::writeConfig()
{
    QStringList groups = m_styles.keys();
    #define E(str) sub->writeEntry(#str, ac.str);
    foreach(QString str, groups)
    {
        KConfigGroup *sub = new KConfigGroup(&*m_config, str);
        ArtisticComment& ac(m_styles[str]);
        E(begin)
        E(end)
        E(lineBegin)
        E(lineEnd)
        E(textBegin)
        E(textEnd)
        sub->writeEntry("lfill", QString(ac.lfill));
        sub->writeEntry("rfill", QString(ac.rfill));
        E(minfill)
        E(realWidth)
        E(truncate)
        switch (ac.type)
        {
        case ArtisticComment::LeftNoFill:
            sub->writeEntry("type", "LeftNoFill");
            break;
        case ArtisticComment::Left:
            sub->writeEntry("type", "Left");
            break;
        case ArtisticComment::Center:
            sub->writeEntry("type", "Center");
            break;
        case ArtisticComment::Right:
            sub->writeEntry("type", "Right");
            break;
        }
    }
    #undef E
}

QStringList ArtisticComment::styles()
{
    return m_styles.keys();
}

ArtisticComment& ArtisticComment::style(const QString& name)
{
    return m_styles[name];
}


void ArtisticComment::setStyle(const QString& name, const ArtisticComment& style)
{
    m_styles[name] = style;
    writeConfig();
}

QString ArtisticComment::decorate(const QString& name, const QString& text)
{
    return m_styles[name].apply(text);
}
