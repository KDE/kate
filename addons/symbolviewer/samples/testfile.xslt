<?xml version="1.0" encoding="UTF-8"?>
<!-- Comment one line -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:variable name="var1">Variable 1</xsl:variable>
    <xsl:param name="param1" select="." />
    <xsl:template match="body">
        <html>
            <body>
                <xsl:apply-templates />
            </body>
        </html>
    </xsl:template>
<!--
Comment multilines
-->
    <xsl:template match="header1">
        <xsl:variable name="var2_in_header1">Variable 2 in template header1</xsl:variable>
        <h1>
            <xsl:apply-templates />
        </h1>
    </xsl:template>
</xsl:stylesheet>
