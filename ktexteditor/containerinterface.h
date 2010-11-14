/* This file is part of the KDE libraries
   Copyright (C) 2007 Philippe Fremy (phil at freehackers dot org)
   Copyright (C) 2008 Joseph Wenninger (jowenn@kde.org)


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

#ifndef KDELIBS_KTEXTEDITOR_CONTAINER_INTERFACE_H
#define KDELIBS_KTEXTEDITOR_CONTAINER_INTERFACE_H

#include <ktexteditor/ktexteditor_export.h>

#include <QtCore/QObject>

namespace KTextEditor
{

class Document;
class View;


/**
 * \brief Class that allows the kpart host to provide some extensions.
 *
 * \ingroup kte_group_editor_extensions
 *
 * The KTextEditor framework allows the kpart host to provide additional
 * services to the kpart. Those services are provided through the
 * ContainerInterface class.
 *
 * If the container supports those specific services, it should set an
 * instance of the service class with ContainerInterface::setContainer(). That
 * instance should inherit QObject, have the Q_OBJECT macro and declare a
 * Q_INTERFACES(), in order for the qobject_cast mechanism to work.
 *
 * To obtain a ContainerInterface, in order to set a specific container
 * extension, the kpart host should do:
 * \code
 * // inside the kpart host
 * Editor * editor = KTextEditor::EditorChooser::editor();
 * ContainerInterface * iface = qobject_cast<ContainerInterface *>( editor );
 * if (iface != NULL) {
 *   iface->setContainer( myContainerExtension );
 * } else {
 *   // the kpart does not support ContainerInterface.
 * }
 * \endcode
 *
 * It is then up to the kpart to use it.
 *
 */
class KTEXTEDITOR_EXPORT ContainerInterface
{
  public:
    /**
     * Constructor.
     */
    ContainerInterface();

    /** Virtual Destructor */
    virtual ~ContainerInterface();

    /**
     * Set the KTextEditor container.
     *
     * This method is used by the KTextEditor host to set an instance
     * of a class providing optional container extensions.
     *
     * \sa container
     */
    virtual void setContainer( QObject * container ) = 0;

    /**
     * Fetch the container extension.
     *
     * This method is used by the KTextEditor component to know
     * which extensions are supported by the KTextEditor host.
     *
     * The kpart will cast the result with qobject_cast() to the right
     * container extension to see if that particular extension is supported:
     *
     * <b>Example:</b>
     * \code
     * // inside the kpart
     *
     * Editor * editor = KTextEditor::EditorChooser::editor();
     * ContainerInterface * iface = qobject_cast<ConainterInterace *>( editor );
     * SomeContainerExtension * myExt =
     *     qobject_cast<SomeContainerExtension *>( iface->container() );
     *
     * if (myExt) {
     *     // do some stuff with the specific container extension
     *     // ...
     * } else {
     *     // too bad, that extension is not supported.
     * }
     * \endcode
     *
     * \sa setContainer
     */
    virtual QObject * container() = 0;
}; // class ContainerInterface

/**
 * A container for MDI-capable kpart hosts.
 *
 * The kpart container for the KTextEditor interface may have different
 * capabilities. For example, inside KDevelop or Kate, the container can
 * manage multiple views and multiple documents. However, if the kpart text
 * is used inside konqueror as a replacement of the text entry in html
 * forms, the container will only support one view with one document.
 *
 * This class allows the kpart component to create and delete views, create
 * and delete documents, fetch and set the current view. Note that the
 * ktexteditor framework already supports multiple document and views and
 * that the kpart host can create them and delete them as it wishes. What
 * this class provides is the ability for the <i>kpart component</i> being
 * hosted to do the same.
 *
 * An instance of this extension should be set with
 * ContainerInterface::setContainerExtension().Check ContainerInterface() to
 * see how to obtain an instance of ContainerInterface. The instance should
 * inherit QObject, inherit MdiContainer, declare the Q_OBJECT macro and
 * declare a Q_INTERFACES(KTextEditor::MdiContainer) .
 *
 * Code example to support MdiContainer (in the kpart host):
 * \code
 * class MyMdiContainer : public QObject,
 *                        public MdiContainer
 * {
 *   Q_OBJECT
 *   Q_INTERFACES( KTextEditor::MdiContainer )
 *
 *   public:
 *     // ...
 * }
 * \endcode
 *
 *
 * To check if the kpart hosts supports the MDI container:
 * \code
 * Editor * editor = KTextEditor::EditorChooser::editor();
 * ContainerInterface * iface = qobject_cast<ContainerInterface *>( editor );
 * if (iface) {
 *   MdiContainer * mdiContainer = qobject_cast<MdiContainer *>( iface->container() );
 *   if (MdiContainer != NULL ) {
 *    // great, I can create additional views
 *    // ...
 *   }
 * }
 * \endcode
 */
class KTEXTEDITOR_EXPORT MdiContainer
{
  public:

    /** Constructor */
    MdiContainer();

    /** Virtual destructor */
    virtual ~MdiContainer();

    /**
     * Set the \p view requested by the part as the active view.
     *
     * \sa activeView
     */
    virtual void setActiveView( View * view )=0;

    /**
     * Get the current activew view.
     *
     * \return the active view.
     *
     * \sa setActiveView
     */
    virtual View * activeView()=0;

    /**
     * Create a new Document and return it to the kpart.
     *
     * Canonical implementation is:
     * \code
     * Document * createDocument()
     * {
     *     Document * doc;
     *     // set parentQObject to relevant value
     *     doc = editor->createDocument( parentQObject );
     *     // integrate the new document in the document list of the
     *     // container, ...
     *     return doc;
     * }
     * \endcode
     *
     * The signal documentCreated() will be emitted during the creation.
     *
     * \return a pointer to the new Document object.
     */
    virtual Document * createDocument()=0;

    /**
     * Closes of document \p doc .
     *
     * The document is about to be closed but is still valid when this
     * call is made. The Document does not contain any view when this
     * call is made (closeView() has been called on all the views of the
     * document previously).
     *
     * The signal aboutToClose() is emitted before this method is
     * called.
     *
     * \return true if the removal is authorized and acted, or
     *     false if removing documents by the kpart is not supported
     *     by the container.
     */
    virtual bool closeDocument( Document * doc )=0;

    /**
     * Creates a new View and return it to the kpart.
     *
     * Canonical implementation is:
     * \code
     * View * createView( Document * doc )
     * {
     *     // set parentWidget to relevant value
     *     return doc->createView( parentWidget );
     * }
     * \endcode
     *
     * The signal viewCreated() will be emitted during the createView()
     * call.
     *
     * \return a pointer to the new View created.
     */
    virtual View * createView( Document * doc )=0;

    /**
     * Closes the View \p view .
     *
     * The view is still valid when this call is made but will be deleted
     * shortly after.
     *
     * \return true if the removal is authorized and acted, or
     *     false if the container does not support view removing from
     *     the kpart, or
     */
    virtual bool closeView( View * view )=0;
}; // class MdiContainer


/**
 * An application providing a centralized place for horizontal view bar containers (eg search bars) has
 * to implement this
 * @since 4.2
 */
class KTEXTEDITOR_EXPORT ViewBarContainer
{
  public:
    enum Position{LeftBar=0,TopBar=1,RightBar=2,BottomBar=3};
    /** Constructor */
    ViewBarContainer();

    /** Virtual destructor */
    virtual ~ViewBarContainer();

    /** At this point the views parent window has to be already set, so this has to be called after any reparentings
    * eg.: The implementation in Kate uses view->window() to determine where to place of the container
    * if 0 is returned, the view has to handle the bar internally
    */
    virtual QWidget* getViewBarParent(View *view,enum Position position)=0;

    /** It is advisable to store only QPointers to the bar and its children in the caller after this point.
     *  The container may at any point delete the bar, eg if the container is destroyed
     *  The caller has to ensure that bar->parentWidget() is the widget returned by the previous function
     */
    virtual void addViewBarToLayout(View *view,QWidget *bar,enum Position position)=0;
    
    ///show hide a view bar. The implementor of this interface has to take care for not showing 
    /// the bars of unfocused views, if needed
    virtual void showViewBarForView(View *view,enum Position position)=0;
    virtual void hideViewBarForView(View *view,enum Position position)=0;

    /**
     * The view should not delete the bar by itself, but tell the container to delete the bar.
     * This is for instance useful, in the destructor of the view. The bar must not life longer
     * than the view.
     */
    virtual void deleteViewBarForView(View *view,enum Position position)=0;

};

} // namespace KTextEditor

Q_DECLARE_INTERFACE(KTextEditor::ContainerInterface, "org.kde.KTextEditor.ContainerInterface")
Q_DECLARE_INTERFACE(KTextEditor::MdiContainer, "org.kde.KTextEditor.MdiContainer")
Q_DECLARE_INTERFACE(KTextEditor::ViewBarContainer, "org.kde.KTextEditor.ViewBarContainer")
#endif // KDELIBS_KTEXTEDITOR_CONTAINER_EXTENSION_H
