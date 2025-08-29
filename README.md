# opc-parser
CLI tool to extract text from OOXML

## usage

```
opc-parser -i example.docx -o example.json

 -i path: document to parse
 -o path: text output (default=stdout)
 - : use stdin for input
```

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


