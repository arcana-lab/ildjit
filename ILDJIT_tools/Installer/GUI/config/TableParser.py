from PyQt4 import QtCore
import urllib, sgmllib,re
class TableParser(sgmllib.SGMLParser):
    '''Parses the sourceforge page in order to get the package's version.'''
    def __init__(self, verbose=0):
        sgmllib.SGMLParser.__init__(self, verbose)
        self.name_exp="/project/showfiles\.php\?group_id=([0-9]{6})&package_id=([0-9]{6})$"
        self.version_exp="/project/showfiles\.php\?group_id=([0-9]{6})&package_id=([0-9]{6})&release_id=([0-9]{6})$"
        self.packId=None
        self.match=False
        self.names=[]
        self.vers=[]
#-----------------------------------------------------------
    def addVersions(self, tree):
        '''Start the parsing and add the versions to the nodes in tree.
        @param  tree: the tree to be completed'''
        for url in tree.versionUrl:
            f = urllib.urlopen(url[0])
            s=f.read()
            self.feed(s)
            self.close()
            tree.root=self.setTreeVersions(tree.root)
        return tree.root
#-----------------------------------------------------------
    def start_a(self, attributes):
        '''Start the parsing of an element in the html file.'''
        for name, value in attributes:
            if name == "href":
                if re.match(self.name_exp, value) != None:
                   self.packId=self.getPackId(value)
                elif re.match(self.version_exp, value) !=None:
                    if self.packId==self.getPackIdFromVersion(value):
                        self.match=True
                        self.packId=None
                    else:
                        self.packId=None
#-----------------------------------------------------------
    def end_a(self):
        '''End the parsing of an element in the html file.'''
        self.match=False
#-----------------------------------------------------------
    def handle_data(self, data):
        '''Perfom the required operation on the data in the html file'''
        if self.packId !=None:
            data=data.strip()
            if data != "":
                self.names.append(data)
        if self.match == True:
            self.vers.append(data)
#-----------------------------------------------------------
    def getPackId(self,string):
        '''@return: the sourceforge id for a package.'''
        return string.split("&package_id=")[1]
#-----------------------------------------------------------
    def getPackIdFromVersion(self,string):
        '''@return: the sourceforge id for a package.'''
        temp=string.split("&package_id=")[1]
        return temp.split("&release_id=")[0]
#-----------------------------------------------------------
    def setTreeVersions(self,node):
        '''Set the versions of all the nodes in the subtree.
        @param node: the root of the subtree to be considered
        @return: the root of the upgraded tree.'''
        temp = dict(zip(self.names,self.vers))
        self.__setTreeVersions(node,temp)
        return node
#-----------------------------------------------------------
    def __setTreeVersions(self,node,temp):
        '''Recursive method to set the version of every node in the tree.
        @param node: root of the subtree to be considered
        @param temp: dictionary containing all the versions of the nodes'''
        try:
            node.version=temp[node.name]
        except:
            pass
        for son in node.sons:
            self.__setTreeVersions(son,temp)