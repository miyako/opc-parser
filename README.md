# opc-parser
CLI tool to extract text from OOXML

## usage

```
opc-parser -i example.docx -o example.json

 -i path: document to parse
 -o path: text output (default=stdout)
 - : use stdin for input
```

## specification

|type|page|paragraph|run|
|-|-|:-:|:-:|
|xlsx|`sheet%d.xml`|`row`|`c/v` where `@t=s\|str\|b\|e\|n` or `c/is/t` where `@t=inlineStr`|
|pptx|`slide%d.xml`|`p`|`t` or `a:t`|


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

**docx**: only an explicit `<br type="page">` starts a new page. it many not be the physical page number.  
**xlsx**: each row is a paragraph. cell values are joined by space ` `.  

**TODO**: handle `<w:sectPr>` in docx. 

