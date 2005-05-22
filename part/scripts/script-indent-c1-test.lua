function indentChar(c)
	katedebug("LUA indentChar has been called")
	local tabWidth = 4
	local spaceIndent = true
	local indentWidth = 4

	local line=view.cursorLine()
	local col=view.cursorColumn()
	local textLine=document.textLine(line)
	local prevLine=document.textLine(line-1);
	
	--local prevIndent=prevLine.match("^%s*")
	--local addIndent=""


    -- unindent } and {, if not in a comment

	if not(string.find(textLine,"^%s*//")) then
		katedebug("no comment")
		katedebug(c);
		if (c=="}") or (c=="{") then
			katedebug("} or { found");
			if (string.find(textLine,"^%s%s%s%s")) then
				katedebug("removing one indentation level");
				document.removeText(line,0,line,tabWidth)
				view.setCursorPositionReal(line,col-tabWidth)
			else
				katedebug("no indentation found");
			end
		end
	else
		katedebug("in comment");
	end
end





function indentNewLine()
	local tabWidth = 4;
	local spaceIndent = true;
	local indentWidth = 4;

	local strIndentCharacters = "    ";
	local strIndentFiller = "";

	local intStartLine, intStartColumn= view.cursorPosition();

	local strTextLine = document.textLine( intStartLine  );
	local strPrevLine = document.textLine( intStartLine  - 1 );

	local addIndent = "";


        local intCurrentLine = intStartLine;
        local openParenCount = 0;
        local openBraceCount = 0;



	function firstNonSpace( text )
		local pos=string.find(text,"[^%s]")
		if pos then
			return pos
		else
			return -1
		end
	end

	function lastNonSpace (text)
		local pos=string.find(text,"[^%s]%s*$")
		if pos then
			return pos
		else
			return -1
		end
	end

	local strCurrentLine=""
	local intCurrentLine=intStartLine
	katedebug(intStartLine .. "-" .. intStartColumn)
	function label_while()
		katedebug("label_while has been entered: intCurrentLine" .. intCurrentLine)
		while (intCurrentLine>0) do
			--katedebug("intCurrentLine:" ..intCurrentLine)
			intCurrentLine=intCurrentLine-1
			strCurrentLine=document.textLine(intCurrentLine)
			intLastChar= lastNonSpace(strCurrentLine)
			intFirstChar=firstNonSpace(strCurrentLine)
			if not (string.find(strCurrentLine,"//")) then
				for intCurrentChar=intLastChar,intFirstChar,-1 do
					ch=string.sub(strCurrentLine,intCurrentChar,intCurrentChar)
					if (ch=="(") or (ch=="[") then
						openParenCount=openParenCount+1
						if (openParenCount>0) then
							return label_while()
						end
					elseif (ch==")") or (ch=="]") then
						openParenCount=openParenCount-1
					elseif (ch=="{") then
						openBraceCount=openBraceCount+1
						if openBraceCount>0 then
							return label_while()
						end
					elseif (ch=="}") then
						openBraceCount=openBraceCount-1
					elseif (ch==";") then
						if (openParenCount==0) then
							lookingForScopeKeywords=false
						end
					end
				end
			end
		end
	end

	label_while()



        katedebug( "line: " .. intCurrentLine)
        katedebug( openParenCount .. ", " .. openBraceCount)

	local ok,match=pcall(function () return string.sub(strCurrentLine,string.find(strCurrentLine,"^%s+")) end)
	if ok then
		katedebug("Line HAD leading whitespaces")
		strIndentFiller=match
	else
		katedebug("Line had NO leading whitespaces")
		strIndentFiller=""
	end


        while( openParenCount > 0 ) do
            openParenCount=openParenCount-1
            strIndentFiller = strIndentFiller .. strIndentCharacters
	end

        while( openBraceCount > 0 ) do
            openBraceCount=openBraceCount-1;
            strIndentFiller = strIndentFiller .. strIndentCharacters
	end

    document.insertText( intStartLine, 0, strIndentFiller )
    view.setCursorPositionReal( intStartLine, string.len(document.textLine( intStartLine )))
end





indenter.register(indenter.OnChar,indentChar)
indenter.register(indenter.OnNewline,indentNewLine)