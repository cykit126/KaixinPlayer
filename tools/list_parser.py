# coding: utf-8

import sys
import codecs

def parse_item(a_line):
    segments = a_line.split('|')
    name_zh = segments[1].strip()
    name_en = segments[2].strip()
    uri = segments[5].strip()
    area_id = segments[10].strip()
    return name_zh,name_en,uri,area_id

if __name__ == '__main__':
    if (len(sys.argv) < 3):
        print("usage: python list_converter.py file")
        exit(1)

    content = u"<channels type=\"com.kaixindev.kxplayer.Channel[]\">\n"
        
    file_path = sys.argv[1]
    out_path = sys.argv[2]
    with codecs.open(file_path, "r", "utf-8") as f:
        for line in f:
            name_zh,name_en,uri,area_id = parse_item(line)
            content = content + u"    <item>\n"
            content = content + u"        <areaId type=\"String\">%s</areaId>\n" % area_id
            content = content + u"        <uri type=\"String\">%s</uri>\n" % uri
            content = content + u"        <name type=\"com.kaixindev.core.I18NString\">\n"
            content = content + u"            <en type=\"String\">%s</en>\n" % name_en
            content = content + u"            <zh type=\"String\">%s</zh>\n" % name_zh
            content = content + u"        </name>\n"
            content = content + u"    </item>\n"

    content = content + u"</channels>\n"

    with codecs.open(out_path, "w", "utf-8") as f:
        f.write(content)
