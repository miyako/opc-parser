# opc-parser
CLI tool to extract text from OOXML

## usage

```
opc-parser -i example.docx -o example.json

 -i path: document to parse
 -o path: text output (default=stdout)
 - : use stdin for input
```

"" output (JSON)

* type: docx, xlsx, pptx
* pages[]
* pages[].index 
* pages[].paragraphs[]


