//
//  main.cpp
//  opc-parser
//
//  Created by miyako on 2025/08/29.
//

#include "opc-parser.h"

static void usage(void)
{
    fprintf(stderr, "Usage:  opc-parser -i in -o out -\n\n");
    fprintf(stderr, "text extractor for ooxml documents\n\n");
    fprintf(stderr, " -%c path: %s\n", 'i' , "document to parse");
    fprintf(stderr, " -%c path: %s\n", 'o' , "text output (default=stdout)");
    fprintf(stderr, " %c : %s\n", '-' , "use stdin for input");
    
    exit(1);
}

extern OPTARG_T optarg;
extern int optind, opterr, optopt;

#ifndef __GNUC__
optarg = 0;
opterr = 1;
optind = 1;
optopt = 0;
int getopt(int argc, OPTARG_T *argv, OPTARG_T opts) {

    static int sp = 1;
    register int c;
    register OPTARG_T cp;
    
    if(sp == 1)
        if(optind >= argc ||
             argv[optind][0] != '-' || argv[optind][1] == '\0')
            return(EOF);
        else if(wcscmp(argv[optind], L"--") == NULL) {
            optind++;
            return(EOF);
        }
    optopt = c = argv[optind][sp];
    if(c == ':' || (cp=wcschr(opts, c)) == NULL) {
        ERR(L": illegal option -- ", c);
        if(argv[optind][++sp] == '\0') {
            optind++;
            sp = 1;
        }
        return('?');
    }
    if(*++cp == ':') {
        if(argv[optind][sp+1] != '\0')
            optarg = &argv[optind++][sp+1];
        else if(++optind >= argc) {
            ERR(L": option requires an argument -- ", c);
            sp = 1;
            return('?');
        } else
            optarg = argv[optind++];
        sp = 1;
    } else {
        if(argv[optind][++sp] == '\0') {
            sp = 1;
            optind++;
        }
        optarg = NULL;
    }
    return(c);
}
#endif

static void extract_text_nodes(xmlNode *node, std::string& text) {
    
    for (xmlNode *cur = node; cur; cur = cur->next) {
        if (
            (cur->type == XML_ELEMENT_NODE)
            &&
              ((!xmlStrcmp(cur->name, (const xmlChar *)"t"))
            || (!xmlStrcmp(cur->name, (const xmlChar *)"a:t")))
            ) {
            xmlChar *content = xmlNodeGetContent(cur);
            if (content) {
                text += (char *)content;
                xmlFree(content);
            }
        }
        extract_text_nodes(cur->children, text);
    }
}

struct Run {
    std::string text;
};

struct Paragraph {
    std::vector<Run> runs;
};

struct Page {
    std::vector<Paragraph> paragraphs;
};

struct Document {
    std::string type;
    std::vector<Page> pages;
};

static void document_to_json(Document& document, std::string& text) {
    
    Json::Value documentNode(Json::objectValue);
    documentNode["type"] = document.type;
    documentNode["pages"] = Json::arrayValue;
    
    for (const auto &page : document.pages) {
        Json::Value pageNode(Json::objectValue);
        Json::Value paragraphsNode(Json::arrayValue);
        
        for (const auto &paragraph : page.paragraphs) {
            std::string _text;
            for (const auto &run : paragraph.runs) {
                _text += run.text;
            }
            if(_text.length() != 0){
                paragraphsNode.append(_text);
            }
        }
        pageNode["paragraphs"] = paragraphsNode;
        documentNode["pages"].append(pageNode);
    }
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    text = Json::writeString(writer, documentNode);
}

static std::string node_text(xmlNodePtr node) {
                            
    if (!node) return "";
    xmlChar* t = xmlNodeGetContent(node);
    if (!t) return "";
    std::string s = (const char*)t;
    xmlFree(t);
                            
    return s;
}

static xmlNodePtr find_first_child(xmlNodePtr parent, const char* localname) {
                            
    for (xmlNodePtr c = parent ? parent->children : nullptr; c; c = c->next) {
        if (c->type == XML_ELEMENT_NODE && strcmp((const char*)c->name, localname) == 0) return c;
    }
    return nullptr;
}

static std::string xml_get_text_concat(xmlNodePtr node, const char* elem_localname) {
                            
    // Concatenate all descendant <elem_localname> text nodes (e.g., <t> inside rich text)
    std::string out;
    std::vector<xmlNodePtr> stack;
    if (node) stack.push_back(node);
    while (!stack.empty()) {
        xmlNodePtr cur = stack.back(); stack.pop_back();
        if (cur->type == XML_ELEMENT_NODE && strcmp((const char*)cur->name, elem_localname) == 0) {
            xmlChar* txt = xmlNodeGetContent(cur);
            if (txt) { out.append((const char*)txt); xmlFree(txt); }
        }
        for (xmlNodePtr c = cur->children; c; c = c->next) stack.push_back(c);
    }
    return out;
}

static std::string resolve_inline_str(xmlNodePtr c_node) {
                            
    // inlineStr → child <is> rich text with one or more <t>
    xmlNodePtr is = find_first_child(c_node, "is");
    if (!is) return "";
    return xml_get_text_concat(is, "t");
}

static std::string resolve_cell_value(xmlNodePtr c_node, const std::vector<std::string>& sst) {
    // <c t="s|inlineStr|str|b|e|n"><v>…</v> or <is>…</is></c>
    std::string t_attr;
    if (xmlChar* t = xmlGetProp(c_node, (const xmlChar*)"t")) {
        t_attr = (const char*)t; xmlFree(t);
    }

    if (t_attr == "inlineStr") {
        return resolve_inline_str(c_node);
    }

    // Find <v>
    xmlNodePtr v = find_first_child(c_node, "v");
    std::string vtext = node_text(v);

    if (t_attr == "s") {
        // shared string index
        if (!vtext.empty()) {
            long idx = strtol(vtext.c_str(), nullptr, 10);
            if (idx >= 0 && idx < (long)sst.size()) return sst[(size_t)idx];
        }
        return "";
    }
    if (t_attr == "str") {
        return vtext; // formula string result
    }
    if (t_attr == "b") {
        // boolean 0/1
        return (vtext == "1") ? "TRUE" : "FALSE";
    }
    if (t_attr == "e") {
        // Excel error code (e.g., #DIV/0!)
        return vtext;
    }
    // default numeric (or text if the cell is styled as text; formatting not applied here)
    return vtext;
}

static void process_worksheet(xmlNode *node, Document& document,const char *tag, const std::vector<std::string>& sst) {
    
    Page *page = &document.pages.back();
    
    xmlNodePtr sheetData = nullptr;
    // find sheetData anywhere under root (namespaces don't matter for local names)
    std::vector<xmlNodePtr> stack;
    if (node) stack.push_back(node);
    while (!stack.empty() && !sheetData) {
        xmlNodePtr cur = stack.back(); stack.pop_back();
        if (cur->type == XML_ELEMENT_NODE && strcmp((const char*)cur->name, "sheetData") == 0) {
            sheetData = cur; break;
        }
        for (xmlNodePtr c = cur->children; c; c = c->next) stack.push_back(c);
    }
    
    if(sheetData){
        for (xmlNodePtr row = sheetData->children; row; row = row->next) {
            if(row->type == XML_ELEMENT_NODE){
                if (!xmlStrcmp(row->name, (const xmlChar *)tag)) {
                    Paragraph _paragraph;
                    page->paragraphs.push_back(_paragraph);
                    std::string value;
                    for (xmlNodePtr cell = row->children; cell; cell = cell->next) {
                        if (cell->type != XML_ELEMENT_NODE || strcmp((const char*)cell->name, "c") != 0) continue;
                        std::string cell_value = resolve_cell_value(cell, sst);
                        if(cell_value.length() != 0) {
                            if(value.length() != 0) {
                                value += " ";
                            }
                            value += cell_value;
                        }
                    }
                    if(value.length() != 0) {
                        if(!page->paragraphs.size()) {
                            //should not happen
                            Paragraph _paragraph;
                            page->paragraphs.push_back(_paragraph);
                        }
                        Paragraph *paragraph = &page->paragraphs.back();
                        paragraph->runs.push_back(Run{value});
                    }
                }
            }
        }
    }
}

static void process_document(xmlNode *node, Document& document,const char *tag, document_type mode) {
    
    Page *page = &document.pages.back();
    
    for (xmlNode *cur = node; cur; cur = cur->next) {
        if(cur->type == XML_ELEMENT_NODE){

            if(mode == document_type_docx) {
                if (!xmlStrcmp(cur->name, (const xmlChar *)"br")) {
                    xmlChar* type_attr = xmlGetProp(cur, (const xmlChar*)"type");
                    if (type_attr) {
                        if (!xmlStrcmp(type_attr, (const xmlChar *)"page")) {
//                            std::vector<char>buf(1024);
//                            snprintf(buf.data(), buf.size(), "page%d", (int)document.pages.size() + 1);
                            Page _page;
//                            _page.name = buf.data();
                            document.pages.push_back(_page);
                            page = &document.pages.back();
                        }
                        xmlFree(type_attr);
                    }
                }
            }

            if (!xmlStrcmp(cur->name, (const xmlChar *)tag)) {
                Paragraph _paragraph;
                page->paragraphs.push_back(_paragraph);
            }

            if((mode == document_type_docx) || (mode == document_type_pptx)){
                if((!xmlStrcmp(cur->name, (const xmlChar *)"t"))
                   || (!xmlStrcmp(cur->name, (const xmlChar *)"a:t"))
                   ) {
                    xmlChar *content = xmlNodeGetContent(cur);
                    if (content) {
                        std::string value((const char *)content);
                        if(value.length() != 0) {
                            if(!page->paragraphs.size()) {
                                //should not happen
                                Paragraph _paragraph;
                                page->paragraphs.push_back(_paragraph);
                            }
                            Paragraph *paragraph = &page->paragraphs.back();
                            paragraph->runs.push_back(Run{value});
                        }
                        xmlFree(content);
                    }
                }
            }
        }
        
        if(cur->children) {
            process_document(cur->children, document, tag, mode);
        }
    }
}

static xmlDoc *parse_opc_part(opcContainer *container, opcPart part) {
    
    if(part) {
        std::vector<opc_uint8_t>data;
        data.reserve(BUFLEN);
        std::vector<opc_uint8_t>buf(BUFLEN);
        opcContainerInputStream *stream = opcContainerOpenInputStream(container, part);
        if (stream) {
            opc_uint32_t len = 0;
            while ((len = opcContainerReadInputStream(stream, &buf[0], BUFLEN)) && (len)) {
                data.insert(data.end(), buf.begin(), buf.begin() + len);
            }
            data.push_back('\0');
            opcContainerCloseInputStream(stream);
            return xmlParseMemory((const char*)&data[0], (int)data.size());
        }
    }
    
    return NULL;
}

int main(int argc, OPTARG_T argv[]) {
    
    const OPTARG_T input_path  = NULL;
    const OPTARG_T output_path = NULL;
    
    std::vector<opc_uint8_t>docx_data(0);

    int ch;
    std::string text;
    
    while ((ch = getopt(argc, argv, "i:o:-h")) != -1){
        switch (ch){
            case 'i':
                input_path  = optarg;
                break;
            case 'o':
                output_path = optarg;
                break;
            case '-':
            {
                _fseek(stdin, 0, SEEK_END);
                opc_uint32_t len = (opc_uint32_t)_ftell(stdin);
                _fseek(stdin, 0, SEEK_SET);
                docx_data.resize(len);
                fread(docx_data.data(), 1, docx_data.size(), stdin);
            }
                break;
            case 'h':
            default:
                usage();
                break;
        }
    }
        
    if((!docx_data.size()) && (input_path != NULL)) {
        FILE *f = _fopen(input_path, _rb);
        if(f) {
            _fseek(f, 0, SEEK_END);
            opc_uint32_t len = (opc_uint32_t)_ftell(f);
            _fseek(f, 0, SEEK_SET);
            docx_data.resize(len);
            fread(docx_data.data(), 1, docx_data.size(), f);
            fclose(f);
        }
    }
    
    if(!docx_data.size()) {
        std::cerr << "no input!" << std::endl;
        return 1;
    }
    
    opcContainer *container = opcContainerOpenMem(_X(docx_data.data()), (opc_uint32_t)docx_data.size(), OPC_OPEN_READ_ONLY, NULL);
    
    if(!container){
        std::cerr << "not a valid input!" << std::endl;
        return 1;
    }
    
    Document document;
    document_type type = document_type_unknown;
    opcPart part = OPC_PART_INVALID;
    
    part = opcPartFind(container, _X("/word/document.xml"), NULL, 0);
    if(part) {
        document.type = "docx";
        type = document_type_docx;
        goto reader;
    }
    part = opcPartFind(container, _X("/xl/workbook.xml"), NULL, 0);
    if(part) {
        document.type = "xlsx";
        type = document_type_xlsx;
        goto reader;
    }
    part = opcPartFind(container, _X("/ppt/presentation.xml"), NULL, 0);
    if(part) {
        document.type = "pptx";
        type = document_type_pptx;
        goto reader;
    }
        
    reader:
    
    if(part) {
        switch (type) {
            case document_type_docx:
            {
                xmlDoc *xml_doc = parse_opc_part(container, part);
                if (xml_doc) {
                    xmlNode *doc_root = xmlDocGetRootElement(xml_doc);
                    if(doc_root) {
                        Page _page;
//                        _page.name = "page1";
                        document.pages.push_back(_page);
                        process_document(doc_root, document, "p", type);
                        document_to_json(document, text);
                        xmlFreeDoc(xml_doc);
                    }
                }
            }
                break;
            case document_type_xlsx:
            {
                std::vector<std::string> sst;
                opcPart sharedStrings = opcPartFind(container, _X("/xl/sharedStrings.xml"), NULL, 0);
                if(sharedStrings){
                    xmlDoc *xml_sharedStrings = parse_opc_part(container, sharedStrings);
                    if(xml_sharedStrings) {
                        xmlNode *table_root = xmlDocGetRootElement(xml_sharedStrings);
                        if(table_root) {
                            for (xmlNodePtr si = table_root ? table_root->children : nullptr; si; si = si->next) {
                                if(si->type == XML_ELEMENT_NODE){
                                    if (!xmlStrcmp(si->name, (const xmlChar *)"si")) {
                                        std::string s = xml_get_text_concat(si, "t");
                                        sst.emplace_back(std::move(s));
                                    }
                                }
                            }
                        }
                        xmlFreeDoc(xml_sharedStrings);
                    }
                }
                    
                int i = 0;
                opcPart slide = NULL;
                do {
                    i++;
                    std::vector<char>buf(1024);
                    snprintf(buf.data(), buf.size(), "/xl/worksheets/sheet%d.xml", i);
                    std::string slideName(buf.data());
                    slide = opcPartFind(container, _X(slideName.c_str()), NULL, 0);
                    if(slide){
                        xmlDoc *xml_slide = parse_opc_part(container, slide);
                        if(xml_slide) {
                            xmlNode *slide_root = xmlDocGetRootElement(xml_slide);
                            if(slide_root) {
                                Page _page;
//                                _page.name = slideName;
                                document.pages.push_back(_page);
                                process_worksheet(slide_root, document, "row", sst);
                            }
                            xmlFreeDoc(xml_slide);
                        }
                    }
                } while (slide);
                document_to_json(document, text);
            }
                break;
            case document_type_pptx:
            {
                int i = 0;
                opcPart slide = NULL;
                do {
                    i++;
                    std::vector<char>buf(1024);
                    snprintf(buf.data(), buf.size(), "/ppt/slides/slide%d.xml", i);
                    std::string slideName(buf.data());
                    slide = opcPartFind(container, _X(slideName.c_str()), NULL, 0);
                    if(slide){
                        xmlDoc *xml_slide = parse_opc_part(container, slide);
                        if(xml_slide) {
                            xmlNode *slide_root = xmlDocGetRootElement(xml_slide);
                            if(slide_root) {
                                Page _page;
//                                _page.name = slideName;
                                document.pages.push_back(_page);
                                process_document(slide_root, document, "p", type);
                            }
                            xmlFreeDoc(xml_slide);
                        }
                    }
                } while (slide);
                document_to_json(document, text);
            }
                break;
            default:
                break;
        }
    }
    
    opcContainerClose(container, OPC_CLOSE_NOW);
    
    if(!output_path) {
        std::cout << text << std::endl;
    }else{
        FILE *f = _fopen(output_path, _wb);
        if(f) {
            fwrite(text.c_str(), 1, text.length(), f);
            fclose(f);
        }
    }

    return 0;
}
