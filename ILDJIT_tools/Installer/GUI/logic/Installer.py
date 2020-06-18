from subprocess import *
from config.ConfigManager import Node
from PyQt4 import QtCore
import os
class Installer(QtCore.QThread):
    '''Class which performs the actual installation of ildjit.
    In a specifc thread the installation tree will be parsed and the specified action will be executed on the package.'''
    def __init__(self,node,parent):
        '''@param node : the root of the tree to be installed.
        @param parent : the install widget'''
        QtCore.QThread.__init__(self)
        self.parent=parent
        self.node=node
        self.tree=parent.parent.tree
        #get the environment
        path=Popen("echo $PATH",shell=True,stdout=PIPE,stderr=PIPE).communicate()[0]
        ld_library_path=Popen("echo $LD_LIBRARY_PATH",shell=True,stdout=PIPE,stderr=PIPE).communicate()[0]
        pkg_config_path=Popen("echo $PKG_CONFIG_PATH",shell=True,stdout=PIPE,stderr=PIPE).communicate()[0]
        if pkg_config_path =='\n':
            pkg_config_path=''
        self.environment={  "LD_LIBRARY_PATH":ld_library_path+":"+self.parent.installDir+":"+self.parent.installDir+"/lib:"+self.parent.installDir+"/lib/iljit",
                                            "PKG_CONFIG_PATH":pkg_config_path+":"+self.parent.installDir+"/lib/pkgconfig"+":"+self.parent.installDir+"/lib/pkgconfig",
                                            "PATH":path+":"+self.parent.installDir+"/bin"}
        self.parent.parent.environment=self.environment
    def run(self):
        '''Start the installation'''
        self.list=[]
        self.traverseAndInstall(self.node)
    def traverseAndInstall(self,node):
        '''Perform the installation of all the nodes of the specified subtree.
        At the end of the overall installation the old packages will be performed:
        removal of all the tar packages
        removal of all the old directories
        @param  node: the root of the subtree to be considered'''
        if self.install(node) :
            if len(node.sons) == 0:
                self.emit(QtCore.SIGNAL("subtreeCompleted"))
            elif len(node.sons) ==1:
                self.traverseAndInstall(node.sons[0])
            else:
                for son in node.sons:
                    self.traverseAndInstall(son)
    def install(self,node):
        '''
        Install the specified node.
        According to the node's action the following actions will be performed:
        INSTALL:
        # unpack
        # configure
        # make
        # make install
        UPGRADE:
        # make clean
        # make install
        REBUILD:
        # configure
        # make clean
        # make 
        # make install
        @param node: the node to be installed'''
        if not node.action == Node.NONE:
            if node.action==Node.INSTALL:
                self.list.append(node)
                #decompress
                try:
                    print "Decompress"
                    self.emit(QtCore.SIGNAL("updateAction"),"Package: "+node.name+"  -  Decompressing the package")
                    print 'tar -xvf '+self.parent.sourceDir+'/'+node.getPackName()+' -C '+self.parent.sourceDir
                    result=Popen('tar -xvf'+self.parent.sourceDir+'/'+node.getPackName()+' -C '+self.parent.sourceDir,shell=True,stdout=PIPE,stderr=PIPE).communicate()
                    node.dir=(result[0].split("\n")[0]).split('/')[0]
                    if result[1]!=None and result[1]!='':
                        self.emit(QtCore.SIGNAL("error"),"Unpack Error","Error unpacking "+node.packName+".",result[1])
                        return
                    else:
                        print "decompression completed"
                        self.emit(QtCore.SIGNAL("increaseValue"))
                except Exception, e:
                    self.emit(QtCore.SIGNAL("error"),"Unpack Error","Error unpacking "+node.packName+".",str(e))
                    return
            try:
                os.chdir(self.parent.sourceDir+'/'+node.packName+"-"+node.version)
            except Exception, e:
                self.emit(QtCore.SIGNAL("error"),"Missing Folder","Folder "+node.packName+" is missing.",str(e))
                return False
            if node.action == Node.INSTALL or node.action==Node.REBUILD:
                #configure
                try:
                    print "./configure --prefix="+self.parent.installDir+" "+self.parent.config+" > /dev/null"
                    self.emit(QtCore.SIGNAL("updateAction"),"Package: "+node.name+"  -  Checking for dependencies...")
                    command=Popen("./configure --prefix="+self.parent.installDir+" "+self.parent.config+" > /dev/null",shell=True,stdout=None,stderr=PIPE,env=self.environment)
                    if command.wait()!=0:
                        self.emit(QtCore.SIGNAL("error"),"Error while configuring","A dependancy of "+node.packName+" is missing.",command.communicate()[1])
                        return False
                    else:
                        print "configure completed"
                        self.emit(QtCore.SIGNAL("increaseValue"))
                except Exception, e:
                    self.emit(QtCore.SIGNAL("error"),"Error while configuring","A dependancy of "+node.packName+" is missing.",str(e))
                    return False
            #make clean
            if node.action ==  Node.UPGRADE or node.action==Node.REBUILD:
                try:
                    self.emit(QtCore.SIGNAL("updateAction"),"Package: "+node.name+"  -  Cleaning...")
                    command=Popen("make clean > /dev/null",shell=True,stdout=None,stderr=PIPE,env=self.environment)
                    if command.wait()!=0:
                        self.emit(QtCore.SIGNAL("error"),"Error while cleaning","Error while cleaning "+node.packName+".",command.communicate()[1])
                        return False
                    else:
                        print "make clean completed"
                        self.emit(QtCore.SIGNAL("increaseValue"))
                except Exception, e:
                    self.emit(QtCore.SIGNAL("error"),"Error while cleaning","Error while cleaning "+node.packName+".",str(e))
                    return False
            #make
            if not node.action == Node.UPGRADE:
                try:
                    self.emit(QtCore.SIGNAL("updateAction"),"Package: "+node.name+"  -  Building...")
                    command=Popen("make > /dev/null",shell=True,stdout=None,stderr=PIPE,env=self.environment)
                    if command.wait()!=0:
                        self.emit(QtCore.SIGNAL("error"),"Error while building","Error while building "+node.packName+".",command.communicate()[1])
                        return False
                    else:
                        print "make completed"
                        self.emit(QtCore.SIGNAL("increaseValue"))
                except Exception, e:
                    self.emit(QtCore.SIGNAL("error"),"Error while building","Error while building "+node.packName+".",str(e))
                    return False
            #make install
            try:
                self.emit(QtCore.SIGNAL("updateAction"),"Package: "+node.name+"  -  Installing...")
                command=Popen("make install > /dev/null",shell=True,stdout=None,stderr=PIPE,env=self.environment)
                if command.wait()!=0:
                    self.emit(QtCore.SIGNAL("error"),"Error while installing","Error while installing "+node.packName+".",command.communicate()[1])
                    return False
                else:
                    print "make install completed"
                    self.emit(QtCore.SIGNAL("increaseValue")) 
            except Exception, e:
                self.emit(QtCore.SIGNAL("error"),"Error while installing","Error while installing "+node.packName+".",str(es))
                return False
            return True
        else:
            return True
    def deletePackages(self):
        '''Delete all the tar packages and the old directories.'''
        for node in self.list: 
            try:
                print 'rm -f '+self.parent.sourceDir+"/"+node.getPackName()
                self.emit(QtCore.SIGNAL("updateAction"),"Package: "+node.name+"  -  Removing the package")
                result=Popen('rm -f '+self.parent.sourceDir+"/"+node.getPackName(),shell=True,stdout=PIPE,stderr=PIPE).communicate()
                if result[1]!=None and result[1]!='':
                    self.emit(QtCore.SIGNAL("error"),"Removal Error","Error removing "+node.packName+".",result[1])
                    return
                else:
                    print "tar deletion completed"
                    self.emit(QtCore.SIGNAL("increaseValue"))
            except:
                self.emit(QtCore.SIGNAL("error"),"Removal Error","Error removing "+node.packName+".",str(e))
                return
            self.emit(QtCore.SIGNAL("updateAction"),"Package: "+node.name+"  -  Removing the old Package")
            dirs = [f for f in os.listdir(self.parent.sourceDir)
                        if os.path.isdir(os.path.join(self.parent.sourceDir, f))]
            for dir in dirs:
                name=dir[:dir.rfind('-')]
                version = dir [dir.rfind('-')+1:]
                if name == node.name:
                    if self.tree.isGreater(node.version,version):
                        print 'rm -rf '+self.parent.sourceDir+'/'+dir
                        try:
                            result=Popen('rm -rf '+self.parent.sourceDir+'/'+dir,shell=True,stdout=PIPE,stderr=PIPE).communicate()
                            if result[1]!=None and result[1]!='':
                                self.emit(QtCore.SIGNAL("removalError"),"Removal Error","Error removing "+node.packName+".",result[1])
                                return
                            else:
                                print "folder deletion completed"
                        except:
                            self.emit(QtCore.SIGNAL("removalError"),"Removal Error","Error removing "+node.packName+".",str(e))
                            return
            self.emit(QtCore.SIGNAL("increaseValue"))
        self.emit(QtCore.SIGNAL("completion"))
