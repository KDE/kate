/****************************************************************
		First configuration page
****************************************************************/
function Page1 (parentWidget) {
		this.defaults=function() {
			alert("Defaults has been called");
		}
		this.apply=function() {
			alert("Apply has been called");
		}
		this.reset=function() {
			alert("Reset defaults has been called");
		}
		box=new QVBox(parentWidget);
		this.button1=new QPushButton(box);
		this.button1.text="P1 Button 1";
		this.button1.show();
		this.button2=new QPushButton(box);
		this.button2.text="P1 Button 2";
		this.button2.show();
		box.show();	
}
Page1.name="Page1"
Page1.fullName="Test1/Page1";

/****************************************************************
		Second configuration page
****************************************************************/

function Page2 (parentWidget) {
		box=new QVBox(parentWidget);
		this.button1=new QPushButton(box);
		this.button1.text="P2 Button 1";
		this.button1.show();
		this.button2=new QPushButton(box);
		this.button2.text="P2Button 2";
		this.button2.show();
		box.show();
}
Page2.name="Page2";
Page2.fullName="Test1/Page2";

/****************************************************************
		Third configuration page
****************************************************************/

function Page3 (parentWidget) {
		box=new QVBox(parentWidget);
		this.button1=new QPushButton(box);
		this.button1.text="P3 Button 1";
		this.button1.show();
		this.button2=new QPushButton(box);
		this.button2.text="P3 Button 2";
		this.button2.show();
		box.show();
}
Page3.name="Page3";
Page3.fullName="Test1/Page3";


/****************************************************************
		First toolview
****************************************************************/

function MyToolView1 (mainwindow,parentwidget) {
	parentwidget.caption="This is my first JS Toolview";
	parentwidget.icon=StdIcons.BarIcon("yes");

	this.lv = new K3ListView( parentwidget );

	this.lv.addColumn('Pix');
	this.lv.addColumn('One');
	this.lv.addColumn('Two');
	this.lv.addColumn('Three');

	this.lv.insertItem( StdIcons.BarIcon("no"), 'Something', "Nothing", "Thing" );
	this.lv.insertItem( StdIcons.BarIcon("no"), 'Something', "Nothing", "Thing" );
	this.lv.insertItem( StdIcons.BarIcon("no"), 'Something', "Nothing", "Thing" );
	this.lv.insertItem( StdIcons.BarIcon("no"), 'Something', "Nothing", "Thing" );

	this.changed=function() {
		alert("Item changed");
		KATE.DocumentManager.activeDocument().insertText(0,0,"TEST");
	}
	this.lv.connect(this.lv,'selectionChanged()',this,'changed');


	this.mw=mainwindow;
	this.cleanup=function() {
		alert("Cleanup MyToolView1");
	}

}
MyToolView1.startPosition=KATE.ToolView.Right;
MyToolView1.name="myfirstjstoolview"


/****************************************************************
		Second toolview
****************************************************************/

function MyToolView2 (mainwindow,parentwidget) {
	parentwidget.caption="This is my second JS Toolview";
	parentwidget.icon=StdIcons.BarIcon("no");

	this.lb=new QListBox(parentwidget);
	this.mainwindow=mainwindow;
	this.cleanup=function() {
		alert("Cleanup MyToolView2");
	}
}
MyToolView2.startPosition=KATE.ToolView.Left;
MyToolView2.name="mysecondjstoolview"



/****************************************************************
		NewWindow callback
****************************************************************/

function newWindowCallBack(mainwindow) {
	alert("New Window has been created");
/*
	anotherToolView = function  (mainwindow,parentwidget) {
		parentwidget.caption="This is my third JS Toolview";
		parentwidget.icon=StdIcons.BarIcon("kate");

		this.lb=new QListBox(parentwidget);
		this.mainwindow=mainwindow;
		this.cleanup=function() {
			alert("Cleanup MyToolView3");
		}
	}
	anotherToolView.startPosition=KATE.ToolView.Left;
	anotherToolView.name="mythirdjsoolview"
	mainwindow.createToolView(anotherToolView);*/
}

/****************************************************************
		WindowRemoved callback
****************************************************************/
function windowRemovedCallBack(mainwindow) {
	alert("Window has been removed");
}






/****************************************************************
		Initialization
****************************************************************/

cpc=new Array();
cpc.push(Page1);
cpc.push(Page2);
setConfigPages(cpc);
//setConfigPages(Page3);
//addConfigPage(Page3);

tvc=new Array();
tvc.push(MyToolView1);
tvc.push(MyToolView2);
setWindowConfiguration(tvc,newWindowCallBack,windowRemovedCallBack);
//setWindowConfiguration(MyToolView1,newWindowCallBack,windowRemovedCallBack);

