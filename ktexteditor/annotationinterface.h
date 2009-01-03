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

#include <QtCore/QObject>

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
 * \since 4.1
 * \see KTextEditor::AnnotationInterface, KTextEditor::AnnotationViewInterface
 */
class KTEXTEDITOR_EXPORT AnnotationModel : public QObject
{
  Q_OBJECT
  public:

    virtual ~AnnotationModel() {}

    /**
     * data() is used to retrieve the information needed to present the
     * annotation information from the annotation model. The provider
     * should return useful information for the line and the data role.
     *
     * The following roles are supported:
     * - Qt::DisplayRole - a short display text to be placed in the border
     * - Qt::TooltipRole - a tooltip information, longer text possible
     * - Qt::BackgroundRole - a brush to be used to paint the background on the border
     * - Qt::ForegroundRole - a brush to be used to paint the text on the border
     *
     * \param line the line for which the data is to be retrieved
     * \param role the role to identify which kind of annotation is to be retrieved
     *
     * \returns a QVariant that contains the data for the given role.
     */
    virtual QVariant data( int line, Qt::ItemDataRole role ) const = 0;

  Q_SIGNALS:
    /**
     * The model should emit the signal reset() when the text of almost all
     * lines changes. In most cases it is enough to call lineChanged().
     *
     * \note Kate Part implementation details: Whenever reset() is emitted Kate
     *       Part iterates over all lines of the document. Kate Part searches
     *       for the longest text to determine the annotation border's width.
     *
     * \see lineChanged()
     */
    void reset();

    /**
     * The model should emit the signal lineChanged() when a line has to be
     * updated.
     *
     * \note Kate Part implementation details: lineChanged() repaints the whole
     *       annotation border automatically.
     */
    void lineChanged( int line );
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
 * annotation model, which provides some annotation information for each line
 * in the document.
 *
 * Setting a model for a Document makes the model data available for all views.
 * If you only want to provide annotations in exactly one view, you can use
 * the AnnotationViewInterface directly. See the AnnotationViewInterface for
 * further details. To summarize, the two use cases are
 * - (1) show annotations in all views. This means you set an AnnotationModel
 *       with this interface, and then call setAnnotationBorderVisible() for
 *       each view.
 * - (2) show annotations only in one view. This means to \e not use this
 *       interface. Instead, use the AnnotationViewInterface, which inherits
 *       this interface. This means you set a model for the specific View.
 *
 * If you set a model to the Document \e and the View, the View's model has
 * higher priority.
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
 * \since 4.1
 * \see KTextEditor::AnnotationModel, KTextEditor::AnnotationViewInterface
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
 * The AnnotationViewInterface allows to do two things:
 * - (1) show/hide the annotation border along with the possibility to add actions
 *       into its context menu.
 * - (2) set a separate AnnotationModel for the View: Not that this interface
 *       inherits the AnnotationInterface.
 *
 * For a more detailed explanation about whether you want an AnnotationModel
 * in the Document or the View, read the detailed documentation about the
 * AnnotationInterface.
 *
 * \section annoview_access Accessing the AnnotationViewInterface
 *
 * The AnnotationViewInterface is an extension interface for a
 * View, i.e. the View inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // view is of type KTextEditor::View*
 * KTextEditor::AnnotationViewInterface *iface =
 *     qobject_cast<KTextEditor::AnnotationViewInterface*>( view );
 *
 * if( iface ) {
 *     // the implementation supports the interface
 *     // do stuff
 *     iface->setAnnotationBorderVisible( true );
 * }
 * \endcode
 *
 * \since 4.1
 */
class KTEXTEDITOR_EXPORT AnnotationViewInterface : public AnnotationInterface
{
  public:
    virtual ~AnnotationViewInterface() {}

    /**
     * This function can be used to show or hide the annotation border
     * The annotation border is hidden by default.
     *
     * @param visible if \e true the annotation border is shown, otherwise hidden
     */
    virtual void setAnnotationBorderVisible( bool visible ) = 0;

    /**
     * Checks whether the View's annotation border is visible.
     */
    virtual bool isAnnotationBorderVisible() const = 0;

  //
  // SIGNALS!!!
  //
  public:
    /**
     * This signal is emitted before a context menu is shown on the annotation
     * border for the given line and view.
     *
     * \note Kate Part implementation detail: In Kate Part, the menu has an
     *       entry to hide the annotation border.
     *
     * \param view the view that the annotation border belongs to
     * \param menu the context menu that will be shown
     * \param line the annotated line for which the context menu is shown
     *
     * \see setAnnotationContextMenu()
     */
    virtual void annotationContextMenuAboutToShow( KTextEditor::View* view, QMenu* menu, int line ) = 0;

    /**
     * This signal is emitted when an entry on the annotation border was activated,
     * for example by clicking or double-clicking it. This follows the KDE wide
     * setting for activation via click or double-clcik
     *
     * \param view the view to which the activated border belongs to
     * \param line the document line that the activated posistion belongs to
     */
    virtual void annotationActivated( KTextEditor::View* view, int line ) = 0;

    /**
     * This signal is emitted when the annotation border is shown or hidden.
     *
     * \param view the view to which the border belongs to
     * \param visible the current visibility state
     */
    virtual void annotationBorderVisibilityChanged( KTextEditor::View* view, bool visible ) = 0;

};

}

Q_DECLARE_INTERFACE(KTextEditor::AnnotationInterface, "org.kde.KTextEditor.AnnotationInterface")
Q_DECLARE_INTERFACE(KTextEditor::AnnotationViewInterface, "org.kde.KTextEditor.AnnotationViewInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
