![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/opc-parser)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/opc-parser/total)

### Dependencies and Licensing

* the source code of this CLI tool is licensed under the MIT license.
* see [libopc](https://github.com/freuter/libopc/blob/master/LICENSE) for the licensing of **libopc** (BSD).
 
# opc-parser
CLI tool to extract text from OOXML

```
text extractor for ooxml documents

 -i path  : document to parse
 -o path  : text output (default=stdout)
 -        : use stdin for input
 -r       : raw text output (default=json)
 -p pass  : password
```
## JSON (XLSX)

|Property|Level|Type|Description|
|-|-|-|-|
|document|0|||
|document.type|0|Text||
|document.pages|0|Array|=sheets|
|document.pages[].meta|1| Object ||
|document.pages[].meta.name|1| Text |sheet name|
|document.pages[].paragraphs|1|Array|=rows|
|document.pages[].paragraphs[].values|2|Array|=cells|
|document.pages[].paragraphs[].text|2|Text|JSON representation of .values|

## JSON (PPTX, DOCX)

|Property|Level|Type|Description|
|-|-|-|-|
|document|0|||
|document.type|0|Text||
|document.pages|0|Array|=sheets|
|document.pages[].paragraphs|1|Array|=rows|
|document.pages[].paragraphs[].values|2|Array|=cells|
|document.pages[].paragraphs[].text|2|Text|JSON representation of .values|
