from PyQt4 import QtCore, QtGui
from config.ConfigManager import *
from ui.ClickLineEdit import ClickLineEdit
from logic.Installer import *
from ui.ErrorDialog import ErrorDialog
class InstallWid(QtGui.QWidget):
    '''This widget manages the installation of idjit and shows the user a progress bar.'''
#-----------------------------------------------------------
    def __init__(self,parent):
        QtGui.QWidget.__init__(self,parent)
        self.parent=parent
        self.leafs=None
        self.index=0
        self.width=580
        self.height=300
        self.setupUi()
        self.hide()
#-----------------------------------------------------------
    def setupUi(self):
        self.setMinimumSize(QtCore.QSize(self.width,self.height))
        self.gridLayout = QtGui.QGridLayout(self)
        self.horizontalLayout_2 = QtGui.QHBoxLayout()
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_2.addItem(spacerItem)
        self.startButton = QtGui.QPushButton(self)
        self.startButton.setMinimumSize(QtCore.QSize(0, 50))
        self.horizontalLayout_2.addWidget(self.startButton)
        spacerItem1 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_2.addItem(spacerItem1)
        self.gridLayout.addLayout(self.horizontalLayout_2, 3, 1, 1, 1)
        self.progressBar = QtGui.QProgressBar(self)
        self.progressBar.setMinimumSize(QtCore.QSize(0, 50))
        self.progressBar.setValue(0)
        self.gridLayout.addWidget(self.progressBar, 4, 1, 1, 1)
        self.actionLabel= QtGui.QLabel(self)
        self.actionLabel.setText("")
        self.gridLayout.addWidget(self.actionLabel, 5, 1, 1, 1)
        self.horizontalLayout = QtGui.QHBoxLayout()
        spacerItem2 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem2)
        self.nextButton = QtGui.QPushButton(self)
        self.nextButton.setEnabled(False)
        self.nextButton.setIcon(QtGui.QIcon(":images/forward.png"))
        self.nextButton.setIconSize(QtCore.QSize(32,32))
        self.nextButton.setMinimumSize(QtCore.QSize(0, 50))
        self.horizontalLayout.addWidget(self.nextButton)
        self.gridLayout.addLayout(self.horizontalLayout, 6, 1, 1, 1)
        self.verticalLayout = QtGui.QVBoxLayout()
        self.horizontalLayout_4 = QtGui.QHBoxLayout()
        self.installLabel = QtGui.QLabel(self)
        self.horizontalLayout_4.addWidget(self.installLabel)
        spacerItem3 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_4.addItem(spacerItem3)
        self.installLine=ClickLineEdit(self)
        self.installLine.setMinimumSize(QtCore.QSize(400, 0))
        self.horizontalLayout_4.addWidget(self.installLine)
        self.horizontalLayout_5 = QtGui.QHBoxLayout()
        self.pathLabel = QtGui.QLabel(self)
        self.pathLabel.setText(self.tr("PnetLib Installation Path: "))
        self.horizontalLayout_5.addWidget(self.pathLabel)
        spacerItem3 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_5.addItem(spacerItem3)
        self.pathLine=ClickLineEdit(self)
        self.pathLine.setMinimumSize(QtCore.QSize(400, 0))
        self.horizontalLayout_5.addWidget(self.pathLine)
        self.verticalLayout.addLayout(self.horizontalLayout_5)
        self.verticalLayout.addLayout(self.horizontalLayout_4)
        self.horizontalLayout_5 = QtGui.QHBoxLayout()
        self.configLabel = QtGui.QLabel(self)
        self.horizontalLayout_5.addWidget(self.configLabel)
        spacerItem4 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_5.addItem(spacerItem4)
        self.configLine = QtGui.QLineEdit(self)
        self.configLine.setMinimumSize(QtCore.QSize(400, 0))
        self.horizontalLayout_5.addWidget(self.configLine)
        self.verticalLayout.addLayout(self.horizontalLayout_5)
        self.gridLayout.addLayout(self.verticalLayout, 0, 1, 1, 1)
        self.retranslateUi()
        self.connectAll()
#-----------------------------------------------------------
    def retranslateUi(self):
        '''Set all the texts.'''
        self.configLabel.setText(self.tr("Configuration Options :"))
        self.installLabel.setText(self.tr("Installation Directory :"))
#-----------------------------------------------------------
    def connectAll(self):
        '''Connect all the events.'''
        self.connect(self.startButton, QtCore.SIGNAL("clicked()"),self.start)
        self.connect(self.nextButton, QtCore.SIGNAL("clicked()"),self.next)
#-----------------------------------------------------------
    def next(self):
        '''Proceed to the next widget according the the action to be performed.'''
        if not self.parent.action == self.parent.INSTALL:
            self.parent.moveTo(2)
        else:
            self.parent.moveTo(1)
#-----------------------------------------------------------
    def increaseValue(self):
        '''Increase the value of the progress bar by one.'''
        self.progressBar.setValue(self.progressBar.value()+1)
#-----------------------------------------------------------
    def updateAction(self,action):
        '''Set the label which diplays the current action.'''
        self.actionLabel.setText(action)
#-----------------------------------------------------------
    def manageError(self,title,header,stdErr):
        '''Show an error message.
        @param title : the title of the dialog
        @param header : a general warning message
        @param stdErr : the standard error of the failed command'''
        errorDialog = ErrorDialog(self,self.tr(title),self.tr(header),self.tr(str(stdErr)))
        errorDialog.exec_()
        self.progressBar.setValue(0)
        self.actionLabel.setText("")
        self.startButton.setEnabled(True)
        self.installLine.enabled=True
        self.pathLine.enabled=True
        self.configLine.setEnabled(True)
#-----------------------------------------------------------
    def subtreeCompleted(self):
        '''Manage the completion of a subtree.
        If the method has been called as many times as the number of leafs the deletion of all the packages will begin.'''
        self.index=self.index+1
        if self.index==self.leafs:
            self.inst.deletePackages()
#-----------------------------------------------------------
    def start(self):
        '''Start the thread which performs the installation.'''
        self.startButton.setEnabled(False)
        self.sourceDir=self.parent.sourceDir
        self.installDir=str(self.installLine.text())
        self.pnetDir=str(self.pathLine.text())
        if self.installDir == "" and self.pnetDir=="":
            QtGui.QMessageBox.critical(self,self.tr("A field is blank!"),self.tr("Please specify the installation directory\nand the pnet installation directory."));
            self.startButton.setEnabled(True)
        else:
            self.parent.installDir=self.installDir
            self.parent.pnetDir=self.pnetDir
            self.installLine.enabled=False
            self.pathLine.enabled=False
            self.configLine.setEnabled(False)
            self.leafs=self.parent.tree.getLeafsNumber(self.parent.tree.root)
            self.progressBar.setMaximum(self.computeOpNumber(self.parent.tree))
            self.config = str(self.configLine.text()).strip()
            self.inst = Installer(self.parent.tree.root,self)
            QtCore.QObject.connect(self.inst, QtCore.SIGNAL("updateAction"), self.updateAction, QtCore.Qt.QueuedConnection)
            QtCore.QObject.connect(self.inst, QtCore.SIGNAL("subtreeCompleted"), self.subtreeCompleted, QtCore.Qt.QueuedConnection)
            QtCore.QObject.connect(self.inst, QtCore.SIGNAL("error"), self.manageError, QtCore.Qt.QueuedConnection)
            QtCore.QObject.connect(self.inst, QtCore.SIGNAL("increaseValue"), self.increaseValue, QtCore.Qt.QueuedConnection)
            QtCore.QObject.connect(self.inst, QtCore.SIGNAL("completion"), self.completion, QtCore.Qt.QueuedConnection)
            self.parent.tree.printTree(self.parent.tree.root)
            self.inst.start()
#-----------------------------------------------------------
    def computeOpNumber(self,tree):
        '''Compute the number of simple actions to be performed.
        @param tree : the tree with the nodes to be installed'''
        nodes=tree.getNodes(tree.root)
        up=0
        inst=0
        re=0
        for node in nodes:
            if node.action==Node.UPGRADE:
                up=up+1
            elif node.action==Node.REBUILD:
                re=re+1
            elif node.action==Node.INSTALL:
                inst=inst+1
        x = up*2+inst*6+re*4
        return x
#-----------------------------------------------------------
    def showEvent(self,event):
        '''Set the texts and the icons of the widget according to the action chosen by the user.'''
        if self.parent.action == self.parent.UPGRADE:
            self.startButton.setIcon(QtGui.QIcon(":images/tool.png"))
            self.startButton.setIconSize(QtCore.QSize(32,32))
            self.startButton.setText(self.tr("   Start Upgrade"))
        elif self.parent.action == self.parent.INSTALL:
            self.startButton.setIcon(QtGui.QIcon(":images/bottom.png"))
            self.startButton.setIconSize(QtCore.QSize(32,32))
            self.startButton.setText(self.tr("   Start Installation"))
        elif self.parent.action == self.parent.REBUILD:
            self.startButton.setIcon(QtGui.QIcon(":images/reload.png"))
            self.startButton.setIconSize(QtCore.QSize(32,32))
            self.startButton.setText(self.tr("   Start Rebuild"))
        if self.isVisible() == False:
            self.setVisible(True)
#-----------------------------------------------------------
    def completion(self):
        '''Manages the completion of the installation showing a success message'''
        self.progressBar.setValue(self.progressBar.maximum())
        self.nextButton.setEnabled(True)
        title=""
        msg=""
        if self.parent.action==self.parent.INSTALL:
            title="Installation Completed"
            msg="<font><b>INSTALLATION COMPLETED!</b></font>"
        elif self.parent.action==self.parent.UPGRADE:
            title="uPGRADE Completed"
            msg="<font><b>UPGRADE COMPLETED!</b></font>"
        elif self.parent.action==self.parent.REBUILD:
            title="Rebuild Completed"
            msg="<font><b>REBUILD COMPLETED!</b></font>"
        QtGui.QMessageBox.information(self,self.tr(title),self.tr(msg));