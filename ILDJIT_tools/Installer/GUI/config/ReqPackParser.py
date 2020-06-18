# -*- coding: utf-8 -*-
from xml.sax import handler
from xml.sax import make_parser
from xml.sax.handler import feature_namespaces
class ReqPackParser(handler.ContentHandler):
    '''The class provides support to parse the xml file containing the required software.'''
#------------------------------------------------------------------------
    def startElement(self, name, attrs):
        '''Action performed when entering an xml element.
        The value of the attribute *name* is added the the list'''
        if name == "package":
            self.list.append(attrs.get('name', None))
#--------------------------------------------------------------------------
    def parseXml(self , file_location):
        '''Start the parsing.
        @param file_location: the path of the file with the list of the packages'''
        self.list=[]
        parser = make_parser()
        parser.setFeature(feature_namespaces, 0)
        parser.setContentHandler(self)
        parser.parse(file_location)
#--------------------------------------------------------------------------
    def getList(self):
        '''@return: list of the required software'''
        return self.list
    
