function newWindowCallBack(mainwindow) {
    var ac=mainwindow.actionCollection();
    action = new KAction( ac, 'kjsconsole_show_action' );
    action.text = 'Javascript Console Window';
    action.icon = 'konsole';

    mainwindow.showConsole = function()
    {

	KJSConsole();
    }

    action.connect( action, 'activated()', mainwindow, 'showConsole' );

}

setWindowConfiguration(null,newWindowCallBack,null);

