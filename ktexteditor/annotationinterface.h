/* This file is part of the KDE libraries
   Copyright 2008 Andreas Pakulat <apaku@gmx.de>
   Copyright 2008 Dominik Haumann <dhaumann kde org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_ANNOTATIONINTERFACE_H
#define KDELIBS_KTEXTEDITOR_ANNOTATIONINTERFACE_H

#include <ktexteditor/ktexteditor_export.h>

#include <QObject>

class QMenu;

namespace KTextEditor
{

class View;

/**
 * \brief An model for providing line annotation information
 *
 * \section annomodel_intro Introduction
 *
 * AnnotationModel is a model-like interface that can be implemented
 * to provide annotation information for each line in a document. It provides
 * means to retrieve several kinds of data for a given line in the document.
 *
 * \section annomodel_impl Implementing a AnnotationModel
 *
 * The public interface of this class is loosely based on the QAbstractItemModel
 * interfaces. It only has a single method to override which is the \ref data()
 * method to provide the actual data for a line and role combination.
 *
 */
class KTEXTEDITOR_EXPORT AnnotationModel : public QObject
{
  Q_OBJECT
  public:

    virtual ~AnnotationModel() {}

    /**
     * data() is used to retrieve the information needed to present the
     * annotation information from the annotation model. The provider
     * should return useful information at least for the
     *
     * \param line the line for which the data is to be retrieved
     * \param role the role to identify which kind of annotation is to be retrieved
     *
     * \returns a \ref QVariant that contains the data for the given role. The
     * following roles are supported:
     *
     * \ref Qt::DisplayRole - a short display text to be placed in the border
     * \ref Qt::TooltipRole - a tooltip information, longer text possible
     * \ref Qt::BackgroundRole - a brush to be used to paint the background on the border
     * \ref Qt::ForegroundRole - a brush to be used to paint the text on the border
     */
    virtual QVariant data( int line, Qt::ItemDataRole role ) const = 0;

  Q_SIGNALS:
    virtual void reset() = 0;
    virtual void lineChanged( int line ) = 0;
};


/**
 * \brief A Document extension interface for handling Annotation%s
 *
 * \ingroup kte_group_doc_extensions
 *
 * \section annoiface_intro Introduction
 *
 * The AnnotationInterface is designed to provide line annotation information
 * for a document. This interface provides means to associate a document with a
 * line annotation provider, which provides some annotation information for
 * each line in the document.
 *
 * \section annoiface_access Accessing the AnnotationInterface
 *
 * The AnnotationInterface is an extension interface for a Document, i.e. the
 * Document inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // document is of type KTextEditor::Document*
 * KTextEditor::AnnotationInterface *iface =
 *     qobject_cast<KTextEditor::AnnotationInterface*>( document );
 *
 * if( iface ) {
 *     // the implementation supports the interface
 *     // do stuff
 * }
 * \endcode
 *
 * \section annoiface_usage Using the AnnotationInterface
 *
 * To use the interface simply set your own AnnotationModel via the
 * \ref setAnnotationModel() method.
 *
 * \see KTextEditor::AnnotationModel
 */
class KTEXTEDITOR_EXPORT AnnotationInterface
{
  public:
    virtual ~AnnotationInterface() {}

    /**
     * Sets a new \ref AnnotationModel for this document to provide
     * annotation information for each line.
     *
     * \param model the new AnnotationModel
     */
    virtual void setAnnotationModel( AnnotationModel* model ) = 0;

    /**
     * returns the currently set \ref AnnotationModel or 0 if there's none
     * set
     * @returns the current \ref AnnotationModel
     */
    virtual AnnotationModel* annotationModel() const = 0;

};


/**
 * \brief Annotation interface for the View
 *
 * \ingroup kte_group_view_extensions
 *
 * \section annoview_intro Introduction
 *
 * The AnnotationViewInterface provides means to interact with the
 * annotation border that is provided for annotated documents. This interface
 * allows to react on clicks, double clicks on and provde context menus for the
 * annotation border.
 *
 * \section annoview_access Accessing the AnnotationViewInterface
 *
 * The AnnoationViewInterface is an extension interface for a
 * View, i.e. the View inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // view is of type KTextEditor::View*
 * KTextEditor::AnnotationViewInterface *iface =
 *     qobject_cast<KTextEditor::AnnoationViewInterface*>( view );
 *
 * if( iface ) {
 *     // the implementation supports the interface
 *     // do stuff
 *     iface->setAnnotationBorderVisible( true );
 * }
 * \endcode
 *
 */
class KTEXTEDITOR_EXPORT AnnotationViewInterface : public AnnotationInterface
{
  public:
    virtual ~AnnotationViewInterface() {}

    /**
     * This function can be used to show or hide the annotation border
     * @param visible if true the annotation border is shown, else its hidden.
     */
    virtual void setAnnotationBorderVisible( bool visible ) = 0;

    /**
     * Checks whether the annotation border is visible for the view.
     */
    virtual bool isAnnotationBorderVisible() const = 0;

  //
  // SIGNALS!!!
  //
  public:
    /**
     * This signal is emitted before a context menu is shown on the annotation
     * border for the given line and view.
     * \param view the view that the annotation border belongs to
     * \param menu the context menu that will be shown
     * \param line the annotated line for which the context menu is shown
     *
     * \see setAnnotationContextMenu()
     */
    virtual void annotationContextMenuAboutToShow( View* view, QMenu* menu, int line ) = 0;

    /**
     * This signal is emitted when an entry on the annotation border was activated,
     * for example by clicking or double-clicking it. This follows the KDE wide
     * setting for activation via click or double-clcik
     *
     * \param view the view to which the activated border belongs to
     * \param line the document line that the activated posistion belongs to
     */
    virtual void annotationActivated( View* view, int line ) = 0;

    /**
     * This signal is emitted when the annotation border is shown or hidden.
     *
     * \param view the view to which the border belongs to
     * \param visible the current visibility state
     */
    virtual void annotationBorderVisibilityChanged( View* view, bool visible ) = 0;

};

}

Q_DECLARE_INTERFACE(KTextEditor::AnnotationInterface, "org.kde.KTextEditor.AnnotationInterface")
Q_DECLARE_INTERFACE(KTextEditor::AnnotationViewInterface, "org.kde.KTextEditor.AnnotationViewInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
