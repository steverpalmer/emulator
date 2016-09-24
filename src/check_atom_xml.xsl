<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="/">
    <html>
      <body>
        <h1>Errors</h1>
        <ul>
          <!-- xsl:if test="exactly-one(1,2)">
            <li>Has multiple block0 defined</li>
          </xsl:if -->
          <xsl:for-each select="//memorymap/*">
            <xsl:choose>
              <xsl:when test="@name='block0'">
                <xsl:if test="base!=0">
                  <li>Exected block0 to be based at 0000, found <xsl:value-of select="base"/></li>
                </xsl:if>
              </xsl:when>
            </xsl:choose>
          </xsl:for-each>
        </ul>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
