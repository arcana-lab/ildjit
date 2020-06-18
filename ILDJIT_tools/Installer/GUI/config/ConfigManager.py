# -*- coding: utf-8 -*-
from xml.sax import handler
from xml.sax import make_parser
from xml.sax.handler import feature_namespaces
class ConfigParser(handler.ContentHandler):
    '''The class provides support to parse the xml file containing the required packages.'''
#------------------------------------------------------------------------
    def startElement(self, name, attrs):
        '''Action performed when entering an xml element.
        Every field in the xml element will be read and the corresponding action will be performed
        on the tree.'''
        if name == "repository":
            url=attrs.get('url', None).strip()
            version_url=attrs.get('version_url', None).strip()
            self.tree.url.append((url,[]))
            self.tree.versionUrl.append((version_url,[]))
        elif name == "package":
            name=attrs.get('name', None).strip()
            self.tree.url[-1][1].append(name)
            self.tree.versionUrl[-1][1].append(name)
            if attrs.get('father', None) == None:
                pack_name=attrs.get('pack_name', None).strip()
                type=attrs.get('type', None).strip()
                self.tree.root=Node(name,pack_name,type)
            else:
                father=attrs.get('father', None).strip()
                pack_name=attrs.get('pack_name', None).strip()
                type=attrs.get('type', None).strip()
                self.tree.addNode(father,Node(name,pack_name,type))
#--------------------------------------------------------------------------
    def parseXML(self,tree,file_location):
        '''Start the parsing of the xml document.
        @param file_location : the absolute path to the xml file;
        @param tree : the empty tree to be populated.'''
        self.tree=tree
        parser = make_parser()
        parser.setFeature(feature_namespaces, 0)
        parser.setContentHandler(self)
        parser.parse(file_location)
#--------------------------------------------------------------------------
class Node(object):
    '''A node of the tree.This class contains all the information to identify a package.'''
    INSTALL=0
    UPGRADE=1
    NONE=2
    REBUILD=3
#----------------------------------------------------------
    def __init__(self,name,packName,type,action=0):
        '''
        @param name: logical name used to identify the package;
        @param packName: the name of the package to be installed;
        @param type: the extension of a package;
        @param action: the action to be performed: INSTALL, UPGRAD, NONE, REBUILD, the default action is INSTALL.'''
        self.name=name
        self.type=type
        self.packName=packName
        self.version=None
        self.action=action
        self.sons= []
#----------------------------------------------------------
    def __str__(self):
        if self.version == None:
            return "Name: "+self.name+"   Pack Name:"+self.packName+"   Type: "+self.type+"   Action: %d"%self.action
        elif self.version !=None:
            return "Name: "+self.name+"   Pack Name:"+self.packName+"   Type: "+self.type+"   Version: "+self.version+"   Action: %d"%self.action
#----------------------------------------------------------
    def getDirectoryName(self):
        '''@return: the directory of the package'''
        return self.packName+"-"+self.version
#----------------------------------------------------------
    def getPackName(self):
        '''@return: the name and the extension of package'''
        return self.getDirectoryName()+"."+self.type
#----------------------------------------------------------
class Tree(object):
    '''Structure used to store all the packages to be installed.'''
#----------------------------------------------------------
    def __init__(self,config_file):
        self.node=None
        self.root=None
        self.list=[]
        self.versionUrl=[]
        self.url=[]
        self.leafs=0
        cp = ConfigParser()
        cp.parseXML(self,config_file)
    def number(self):
        '''@return: the number of packages to be installed.'''
        return len(self.getNodes(self.root))
    def getLeafsNumber(self,node):
        '''@param node : the root of the subtree to be considered
        @return: the number of the leafs of the tree.'''
        self.leafs=0
        self.__getLeafsNumber(node)
        return self.leafs
    def __getLeafsNumber(self,node):
        '''Recursive method to support the computation of the leafs number.
        @param node : the root of the subtree to be considered'''
        if len(node.sons)==0:
            self.leafs=self.leafs+1
        else:
            for son in node.sons:
                self.__getLeafsNumber(son)
    def getNodes(self,node):
        '''@param node : the root of the subtree to be considered
        @return: the unsorted list of all the packages.'''
        self.list=[]
        self.__getNodes(node)
        return self.list
    def getSons(self,node):
        '''@param node : the root of the subtree to be considered
        @return:: the list of all the sons of the specified node.'''
        self.list=[]
        for son in node.sons:
            self.__getNodes(son)
        return self.list
    def __getNodes(self,node):
        '''Recursive method to support the population of the list of nodes.
        @param node : the root of the subtree to be considered'''
        self.list.append(node)
        for son in node.sons:
            self.__getNodes(son)
    def addNode(self,father,son):
        '''Add a node to the tree
        @param son : the node to be added
        @param father : the father of the node to be added'''
        self.__getNode(self.root,father)
        self.node.sons.append(son)
#----------------------------------------------------------
    def printTree(self,node):
        print "VERSION_URL   %s"%(self.versionUrl)
        print "REPOSITORY_URL   %s"%(self.url)
        self.__printTree(node)
#----------------------------------------------------------
    def __printTree(self,node,index=0):
        space=" "*index
        print space+str(node)
        index=index+1
        for son in node.sons:
            self.__printTree(son,index)        
#----------------------------------------------------------
    def getNode(self,node,name):
        '''
        @param node : the root of the subtree to be considered
        @param name : the name of the node to be returned
        @return: the node with the specified name'''
        self.__getNode(node, name)
        return self.node
#----------------------------------------------------------
    def __getNode(self,node,name):
        '''Recursive method to support the search of the specified node.
        @param node : the root of the subtree to be considered
        @param name : the name of the node to be returned '''
        if node.name == name:
            self.node=node
        elif len(node.sons)==0 :
            return
        else:
            for son in node.sons:
                self.getNode(son, name)
#----------------------------------------------------------
    def getNodeFromPack(self,node,name):
        '''
        @param node : the root of the subtree to be considered
        @param name : the package name of the node to be returned 
        @return: the node with the specified package name'''
        self.__getNodeFromPack(node, name)
        return self.node
#----------------------------------------------------------
    def __getNodeFromPack(self,node,name):
        '''Recursive method to support the search of the specified node.
        @param node : the root of the subtree to be considered
        @param name : the package name of the node to be returned '''
        if node.packName == name:
            self.node=node
        elif len(node.sons)==0 :
            return
        else:
            for son in node.sons:
                self.getNodeFromPack(son, name)
#----------------------------------------------------------
    def getVersions(self,node):
        '''@param node : the root of the subtree to be considered
        @return: the dictionary with the versions of the packages.
        The dictionary has the following structure:
        {pack_name1:version1,pack_name2:version2,...}'''
        list=self.getNodes(self.root)
        dict={}
        for x in list:
            dict[x.packName]=x.version
        return dict
#----------------------------------------------------------
    def getUrl(self,name):
        '''Return the base url of the specified package.
        @param name : the name of the node to be searched'''
        for pack in self.url:
            try:
                if name in pack[1]:return pack[0]
            except:
                pass
#----------------------------------------------------------
    def manageCompletion(self,result):
        self.emit(QtCore.SIGNAL("completion"),result)
#----------------------------------------------------------
    def isGreater(self,v1,v2):
        '''Compare 2 versions
        @param v1: the first version (a string)
        @param v2: the second version (a string)
        @retur true if v1 is greater than v2'''
        try:
            list1=self.getDigits(v1)
            list2=self.getDigits(v2)
        except:
            return False
        else:
            for index in range(min(len(list1),len(list2))):
                if list1[index] > list2[index]:
                    return True
                elif list1[index] < list2[index]:
                    return False
            if len(list1) > len(list2):
                return True
            else:
                return False
#----------------------------------------------------------
    def getDigits(self,num):
        '''Convert a version code in a list of digits.
        @param  num: the string to be converted
        @return: the list of the string's digits'''
        num=num.split(".")
        if len(num) == 1:
            raise Exception
        else:
            list=[]
            for x in num:
                x=int(x)
                stack=[]
                while (x>=10):
                    stack.append(x%10)
                    x=x/10
                stack.append(x)
                stack.reverse()
                list.extend(stack)
            return list
from PyQt4 import QtCore
from TableParser import TableParser
class NetworkManager(QtCore.QThread):
    '''Manages the connection to the remote site to get the version numbers'''
    def __init__(self,tree):
        QtCore.QThread.__init__(self)
        self.tree=tree
#----------------------------------------------------------
    def run(self):
        '''Perform the connection'''
        tp=TableParser()
        try:
            root=tp.addVersions(self.tree)
        except:
            self.emit(QtCore.SIGNAL("connectionError"))
        else:
            self.tree.root=root
            self.emit(QtCore.SIGNAL("completion"),self.tree)