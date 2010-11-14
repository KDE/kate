function newWindowCallBack(mainwindow) {
    var ac=mainwindow.actionCollection();
    action = new KAction( ac, 'kjsselect_select_action' );
    action.text = 'Select enclosing block';
    //action.icon = 'konsole';
   

    mainwindow.selectIt = function()
    {
	endChars=Array();
	endChars['\"']="\"";
	endChars['(']=")";
	endChars['[']="]";
	endChars['\'']="'";
	endChars['{']="}";
	endChar="";
	av=this.viewManager().activeView();
	d=KATE.DocumentManager.activeDocument();

		lineCnt=d.numLines();
		x=av.cursorColumn();
		y=av.cursorLine();
		line=d.textLine(y);
		sy=y;
		sx=x-1;
		while (true) {
			if (sx<0) {
				sy=sy-1;
				if (sy<0) {
					d.selectAll();
					return;
				}
				line=d.textLine(sy);
				while (line.length==0) {
					sy=sy-1;
					if (sy<0) {
						d.selectAll();
						return;
					}
					line=d.textLine(sy);
				}
				sx=line.length-1;

			}		
			if (
				(line[sx]=="\"") ||
				(line[sx]=="'") ||
				(line[sx]=="(") ||
				(line[sx]=="[") ||
				(line[sx]=="{") 
			) {
				endChar=endChars[line[sx]];
				break;
			}else sx--;
		}


		alert("Searching end");
		ex=x;
		ey=y;
		line=d.textLine(y);
		while (true) {
			if (ex>=(line.length-1)) {
				ey=ey+1;
				if (ey>=lineCnt) {
					d.selectAll();
					return;
				}
				line=d.textLine(ey);
				while (line.length==0) {
					ey=ey+1;
					if (ey>=lineCnt) {
						d.selectAll();
						return;
					}
					line=d.textLine(ey);
				}
				ex=0;
			}		
			if (line[ex]==endChar)
			break; else ex++;
		}
		d.setSelection(sy,sx,ey,ex);
	
    }

    action.connect( action, 'activated()', mainwindow, 'selectIt' );

}

setWindowConfiguration(null,newWindowCallBack,null);

