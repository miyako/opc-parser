![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/opc-parser)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/opc-parser/total)

# opc-parser
CLI tool to extract text from OOXML

## usage

```
opc-parser -i example.docx -o example.json

 -i path: document to parse
 -o path: text output (default=stdout)
 -: use stdin for input
 -r: raw text output (default=json)
```

## specification

|type|page|paragraph|run|
|-|-|:-:|-|
|docx|`br` where `type=page` or `sectPr/type` where `val=nextPage`|`p`|`t` or `a:t`
|xlsx|`sheet%d.xml`|`row`|`c/v` where `@t=s\|str\|b\|e\|n` or `c/is/t` where `@t=inlineStr`|
|pptx|`slide%d.xml`|`p`|`t` or `a:t` cells are join by `\t`|


## output (JSON)

```
{
    "type: "docx" | "xlsx" | "pptx",
    "pages": [
        {
            "paragraphs": [{array of string}]
        }
    ]
}
```
