# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
from ui.FirstWid import FirstWid
from ui.InstallWid import InstallWid
from ui.DownloadWid import DownloadWid
from ui.LastWid import LastWid
from ui.BashWid import BashWid
from ui.AboutILDJITDialog import AboutILDJITDialog
from ui.AboutDialog import AboutDialog
from ui.PackDialog import PackDialog
from config.ReqPackParser import ReqPackParser
import application_rc
import os
class MainWindow(QtGui.QMainWindow):
    '''This class represent the main window of the application. 
    It wraps the central widget and the menu bar.'''
    INSTALL=0
    UPGRADE=1
    REBUILD=2
#-----------------------------------------------------------
    def __init__(self,parent=None):
        QtGui.QMainWindow.__init__(self,parent)
        cd = os.getcwd()
        cd=cd+'/'
        self.CONFIG_FILE=cd+"res/required_packages.xml"
        self.PACK_FILE=cd+"res/required_software.xml"
        self.SOURCE_DIRECTORY=None
        self.INSTALL_DIRECTORY=None
        self.sourceDir=None
        self.installDir=None
        self.pnetDir=None
        self.tree=None
        self.action=None
        self.setupUi()
        self.mainWidget=None
        self.list=[]
        self.list.append(FirstWid(self))
        self.list.append(DownloadWid(self))
        self.list.append(InstallWid(self))
        self.list.append(BashWid(self))
        self.list.append(LastWid(self))
        self.setMainWidget(self.list[0])
        self.setWindowIcon(QtGui.QIcon(":/images/wizard.png"))
#-----------------------------------------------------------
    def setupUi(self):
        self.ad=AboutDialog(self)
        self.ailjitd=AboutILDJITDialog(self)
        self.centralWid=QtGui.QWidget(self)
        self.setCentralWidget(self.centralWid)
        self.mainLayout = QtGui.QGridLayout()
        self.centralWid.setLayout(self.mainLayout)
        self.menubar = QtGui.QMenuBar(self)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 531, 24))
        self.menuHelp = QtGui.QMenu(self.menubar)
        self.menuAbout = QtGui.QMenu(self.menubar)
        self.setMenuBar(self.menubar)
        self.actionRequiredPackages = QtGui.QAction(self)
        self.actionAboutQt = QtGui.QAction(self)
        self.actionAboutILDJIT = QtGui.QAction(self)
        self.actionAboutTheMightyInstaller = QtGui.QAction(self)
        self.menuHelp.addAction(self.actionRequiredPackages)
        self.menuAbout.addAction(self.actionAboutILDJIT)
        self.menuAbout.addAction(self.actionAboutTheMightyInstaller)
        self.menuAbout.addSeparator()
        self.menuAbout.addAction(self.actionAboutQt)
        self.menubar.addAction(self.menuHelp.menuAction())
        self.menubar.addAction(self.menuAbout.menuAction())
        self.retranslateUi()
        self.connectAll()
#-----------------------------------------------------------
    def connectAll(self):
        '''Connect all the events'''
        self.connect(self.actionAboutQt, QtCore.SIGNAL("triggered()"), QtGui.qApp.aboutQt)
        self.connect(self.actionAboutILDJIT, QtCore.SIGNAL("triggered()"), self.ailjitd.show)
        self.connect(self.actionAboutTheMightyInstaller, QtCore.SIGNAL("triggered()"), self.ad.show)
        self.connect(self.actionRequiredPackages, QtCore.SIGNAL("triggered()"), self.showPackDialog)
#-----------------------------------------------------------
    def retranslateUi(self):
        '''Set all the texts'''
        self.setWindowTitle(self.tr("AIKA: The Mighty ILDJIT Installer"))
        self.menuHelp.setTitle(self.tr("&Help"))
        self.menuAbout.setTitle(self.tr("&About"))
        self.actionRequiredPackages.setText(self.tr("&Required Packages"))
        self.actionAboutQt.setText(self.tr("About &Qt"))
        self.actionAboutILDJIT.setText(self.tr("About &ILDJIT"))
        self.actionAboutTheMightyInstaller.setText(self.tr("About The &Mighty Installer"))
#-----------------------------------------------------------
    def setMainWidget(self,wid):
        '''Set the central widget of the window to wid and centres the window in the screen.'''
        self.mainLayout.addWidget(wid)
        self.mainWidget=wid
        event=QtGui.QShowEvent()
        self.mainWidget.showEvent(event)
        rect = QtGui.QApplication.desktop().availableGeometry(self)
        self.move((rect.width()-self.width())/2,(rect.height()-self.height())/2)
        self.setGeometry((rect.width()-wid.width)/2,(rect.height()-wid.height)/2,wid.width,wid.height)
#-----------------------------------------------------------
    def moveTo(self,offset):
        '''Change the main widget. Move with a the specified offset.
        @param offset: the step to be taken backward or forward in the widget list'''
        try:
            index=self.list.index(self.mainWidget)
        except:
            index=1
        self.switch(self.list[index+offset])
#-----------------------------------------------------------
    def switch(self,new):
        '''Remove the current widget and set the new one.
        @param new: the new widget to be set'''
        self.mainWidget.hide()
        self.mainLayout.removeWidget(self.mainWidget);
        self.setMainWidget(new)
#-----------------------------------------------------------
    def showPackDialog(self):
        '''Show the required software dialog.'''
        try:
            parser=ReqPackParser()
            parser.parseXml(self.PACK_FILE)
            list = QtCore.QStringList()
            for x in parser.getList():
                list.append(x)
        except:
            QtGui.QMessageBox.critical(self,self.tr("Parsing Error"), self.tr("Error parsing: "+self.PACK_FILE+"."));
        else:
            packDialog=PackDialog(self,list)
            packDialog.show()
            
