# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
from ClickLineEdit import ClickLineEdit
from DownTableWidget import DownTableWidget
from config.ConfigManager import *
from ui.UpgradeDialog import UpgradeDialog
from ui.ProgressDialog import ProgressDialog
import os
class DownloadWid(QtGui.QWidget):
    '''This widget asks the user the source directory and shows the progress in the download.'''
#-----------------------------------------------------------
    def __init__(self,parent=None):
        QtGui.QWidget.__init__(self,parent) 
        self.parent=parent
        self.width=730
        self.height=600
        self.hide()
        self.setupUi()
#-----------------------------------------------------------
    def setupUi(self):
        self.setMinimumSize(QtCore.QSize(self.width,self.height))
        self.gridLayout = QtGui.QGridLayout(self)
        self.horizontalLayout_3 = QtGui.QHBoxLayout()
        self.sourceLabel = QtGui.QLabel(self)
        self.sourceLabel.setText("Source Directory : ")
        self.horizontalLayout_3.addWidget(self.sourceLabel)
        spacerItem = QtGui.QSpacerItem(40,20,QtGui.QSizePolicy.Expanding,QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_3.addItem(spacerItem)
        self.sourceLine = ClickLineEdit(self,False)
        self.sourceLine.setMinimumSize(QtCore.QSize(400,0))
        self.horizontalLayout_3.addWidget(self.sourceLine)
        self.gridLayout.addLayout(self.horizontalLayout_3,0,0,1,1)
        self.tableWidget = DownTableWidget(self)
        self.gridLayout.addWidget(self.tableWidget,5,0,1,1)
        self.horizontalLayout = QtGui.QHBoxLayout()
        spacerItem3 = QtGui.QSpacerItem(40,20,QtGui.QSizePolicy.Expanding,QtGui.QSizePolicy.Minimum)
        self.backButton = QtGui.QPushButton(self)
        self.backButton.setMinimumSize(QtCore.QSize(0,50))
        self.backButton.setIcon(QtGui.QIcon(":images/back.png"))
        self.backButton.setIconSize(QtCore.QSize(32,32))
        self.horizontalLayout.addWidget(self.backButton)
        self.horizontalLayout.addItem(spacerItem3)
        self.nextButton = QtGui.QPushButton(self)
        self.nextButton.setEnabled(False)
        self.nextButton.setMinimumSize(QtCore.QSize(0,50))
        self.nextButton.setIcon(QtGui.QIcon(":images/forward.png"))
        self.nextButton.setIconSize(QtCore.QSize(32,32))
        self.horizontalLayout.addWidget(self.nextButton)
        self.gridLayout.addLayout(self.horizontalLayout,6,0,1,1)
        self.horizontalLayout_2 = QtGui.QHBoxLayout()
        spacerItem4 = QtGui.QSpacerItem(40,20,QtGui.QSizePolicy.Expanding,QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_2.addItem(spacerItem4)
        self.startButton = QtGui.QPushButton(self)
        self.startButton.setMinimumSize(QtCore.QSize(0,50))
        self.horizontalLayout_2.addWidget(self.startButton)
        spacerItem5 = QtGui.QSpacerItem(40,20,QtGui.QSizePolicy.Expanding,QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_2.addItem(spacerItem5)
        self.gridLayout.addLayout(self.horizontalLayout_2,3,0,1,1)
        self.connectAll()
#----------------------------------------------------------
    def connectAll(self):
        '''Connect all the signals'''
        self.connect(self.backButton, QtCore.SIGNAL("clicked()"),self.prev)
        self.connect(self.startButton, QtCore.SIGNAL("clicked()"),self.start)
        self.connect(self.nextButton, QtCore.SIGNAL("clicked()"),self.next)
#----------------------------------------------------------
    def showEvent(self,event):
        '''Set the texts and the icons of the widget according to the action chosen by the user.'''
        if not event.spontaneous():
            self.sourceLine.setText("")
            self.startButton.setEnabled(True)
            self.sourceLine.enabled=True
            self.nextButton.setEnabled(False)
        self.setVisible(True)
        if self.parent.action == self.parent.INSTALL:
            self.startButton.setIcon(QtGui.QIcon(":images/bottom.png"))
            self.startButton.setIconSize(QtCore.QSize(32,32))
            self.startButton.setText(self.tr("   Start Download"))
        elif self.parent.action == self.parent.UPGRADE:
            self.startButton.setIcon(QtGui.QIcon(":images/tool.png"))
            self.startButton.setIconSize(QtCore.QSize(32,32))
            self.startButton.setText(self.tr("   Check for Updates"))
        elif self.parent.action == self.parent.REBUILD:
            self.startButton.setIcon(QtGui.QIcon(":images/reload.png"))
            self.startButton.setText("   Check for Packages")
            self.startButton.setIconSize(QtCore.QSize(32,32))
#-----------------------------------------------------------
    def start(self):
        '''Perform the right action according to the action chosen by the user'''
        self.sourceDir=str(self.sourceLine.text())
        if self.sourceDir=='':
            QtGui.QMessageBox.critical(self,self.tr("A field is Missing!"), self.tr("The source directory is empty."));
        else:
            self.progress=ProgressDialog(self)
            self.progress.show()
            self.backButton.setEnabled(False)
            self.startButton.setEnabled(False)
            self.sourceLine.enabled=False
            #build the tree
            try:
                self.tree=Tree(self.parent.CONFIG_FILE)
                self.nm = NetworkManager(self.tree)
                self.connect(self.nm, QtCore.SIGNAL("connectionError"), self.manageConnectionError, QtCore.Qt.QueuedConnection)
                if self.parent.action == self.parent.UPGRADE:
                    self.connect(self.nm, QtCore.SIGNAL("completion"), self.check, QtCore.Qt.QueuedConnection)
                elif self.parent.action == self.parent.INSTALL:
                    self.connect(self.nm, QtCore.SIGNAL("completion"), self.startDownload, QtCore.Qt.QueuedConnection)
                elif self.parent.action == self.parent.REBUILD:
                    self.connect(self.nm, QtCore.SIGNAL("completion"), self.checkForRebuild, QtCore.Qt.QueuedConnection)
                self.nm.start()
            except:
                self.progress.action=ProgressDialog.ERROR
                self.progress.title="Parsing Error"
                self.progress.msg="Error parsing the configuration file:\n"+self.parent.CONFIG_FILE
                self.progress.closeEvent(QtGui.QCloseEvent())
                
#-----------------------------------------------------------
    def manageConnectionError(self):
        '''Show an error message to the user to notify a connection error.'''
        self.progress.action=ProgressDialog.ERROR
        self.progress.title="Connection Error"
        self.progress.msg="Cannot connect to the server!"
        self.progress.closeEvent(QtGui.QCloseEvent())
#-----------------------------------------------------------
############################
#INSTALL METHODS
############################    
#-----------------------------------------------------------
    def startDownload(self,tree):
        '''Start the download of all the packages for the installation.
        @param tree: the installation tree. '''
        self.parent.tree=tree
        self.parent.sourceDir=self.sourceDir
        self.progress.action=ProgressDialog.START_DOWN
        self.progress.closeEvent(QtGui.QCloseEvent())
#-----------------------------------------------------------
############################
#UPGRADE METHODS
############################
#-----------------------------------------------------------
    def check(self,tree):
        '''Check if there are new versions of a package. '''
        try:
            old=self.getOldNodes(tree)
        except:
            self.progress.action=ProgressDialog.ERROR
            self.progress.title="Error!"
            self.progress.msg="Check your source directory"
            self.progress.closeEvent(QtGui.QCloseEvent())
        else:
            self.parent.tree=tree
            self.setAction(self.parent.tree.root,False,old)
            if self.parent.action == self.parent.UPGRADE:
                self.list = QtCore.QStringList()
                for x in old:
                    self.list.append(x.name+" - "+x.version)
                self.progress.action=ProgressDialog.CHECK
                self.progress.closeEvent(QtGui.QCloseEvent())
#-----------------------------------------------------------
    def showDialog(self,toUpgrade):
        '''Show a dialog to notify the user the packages to be upgraded.'''
        window = UpgradeDialog(None,toUpgrade)
        if window.exec_()==UpgradeDialog.Accepted:
            self.parent.sourceDir=self.sourceDir
            self.tableWidget.startDownload(self.sourceDir)
            #self.nextButton.setEnabled(True)
        else:
            self.startButton.setEnabled(True)
            self.backButton.setEnabled(True)
#-----------------------------------------------------------
    def getOldNodes(self,tree):
        '''Get the list of the packages to be upgraded.'''
        new=tree.getVersions(tree.root)
        toUpgrade=[]
        old = self.findPacks(tree)
        for pack in new:
            if pack in old:
                if new[pack]!=old[pack]:
                    toUpgrade.append(tree.getNodeFromPack(tree.root,pack)) 
            else:
                toUpgrade.append(tree.getNodeFromPack(tree.root,pack))
        return toUpgrade
#----------------------------------------------------------
    def setAction(self,node,parent,toUpgrade):
        '''Set the action to be performed on every node of the tree.'''
        if self.parent.action == self.parent.REBUILD:
            node.action=Node.REBUILD
            for son in node.sons:
                self.setAction(son,False,None)
        else:
            if node in toUpgrade:
                node.action=Node.INSTALL
                for son in node.sons:
                    self.setAction(son,True,toUpgrade)
            else:
                if parent == True:
                    node.action=Node.UPGRADE
                else:
                    node.action=Node.NONE
                for son in node.sons:
                    self.setAction(son,parent,toUpgrade)
        
#----------------------------------------------------------
    def findPacks(self,tree):
        '''Find the packages to be upgraded.'''
        dirs = [f for f in os.listdir(self.sourceDir)
                    if os.path.isdir(os.path.join(self.sourceDir, f))]
        new=tree.getVersions(tree.root)
        old={}
        for dir in dirs:
            try:
                name=dir.rsplit('-',1)[0]
                version=dir.rsplit('-',1)[1]
            except:
                pass
            else:
                if name in new:
                    if name in old:
                        try:
                            if tree.isGreater(version,old[name]):
                                old[name]=version
                        except:
                            pass
                    else:
                        try:
                            tree.getDigits(version)
                        except:
                            pass
                        else:
                            old[name]=version  
        return old
#----------------------------------------------------------
    def next(self):
        '''Move to the next widget.'''
        self.parent.moveTo(1)
#----------------------------------------------------------
    def prev(self):
        '''Move to the previous widget.'''
        self.parent.moveTo(-1)
#----------------------------------------------------------
############################
#REBUILD METHODS
############################
#-----------------------------------------------------------
    def checkForRebuild(self,tree):
        '''Check if all the required packages are installed.'''
        try:
            old=self.getOldNodes(tree)
        except:
            self.progress.action=ProgressDialog.ERROR
            self.progress.title="Error!"
            self.progress.msg="Check your source directory"
            self.progress.closeEvent(QtGui.QCloseEvent())
        else:
            self.parent.tree=tree
            self.setAction(self.parent.tree.root,False,None)
            if len(old)!=0:
                self.progress.action=ProgressDialog.ERROR
                self.progress.title="Updates Found!"
                self.progress.msg="ILDJIT is outdated or not properly installed.\nTry to perform a new installation or an upgrade."
                self.progress.closeEvent(QtGui.QCloseEvent())
            else:
                self.parent.sourceDir=self.sourceDir
                self.progress.action=ProgressDialog.REBUILD
                self.progress.closeEvent(QtGui.QCloseEvent())