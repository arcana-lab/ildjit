from PyQt4 import QtCore, QtGui
class FirstWid(QtGui.QWidget):
    '''This widget asks the user which action he wants to perform.'''
#-----------------------------------------------------------
    def __init__(self,parent=None):
        QtGui.QWidget.__init__(self,parent) 
        self.parent=parent
        self.width=400
        self.height=500
        self.setupUi()
#-----------------------------------------------------------
    def setupUi(self):
        self.setMinimumSize(QtCore.QSize(self.width,self.height))
        self.gridLayout = QtGui.QGridLayout(self)
        spacerItem = QtGui.QSpacerItem(20,40,QtGui.QSizePolicy.Minimum,QtGui.QSizePolicy.Expanding)
        self.gridLayout.addItem(spacerItem,0,1,1,1)
        spacerItem1 = QtGui.QSpacerItem(40,20,QtGui.QSizePolicy.Expanding,QtGui.QSizePolicy.Minimum)
        self.gridLayout.addItem(spacerItem1,1,0,1,1)
        self.verticalLayout = QtGui.QVBoxLayout()
        self.imageLabel=QtGui.QLabel(self)
        self.imageLabel.setPixmap(QtGui.QPixmap(":images/logo.png"))
        self.verticalLayout.addWidget(self.imageLabel)
        spacerItem2 = QtGui.QSpacerItem(20,40,QtGui.QSizePolicy.Minimum,QtGui.QSizePolicy.Expanding)
        self.verticalLayout.addItem(spacerItem2)
        self.installButton = QtGui.QPushButton(self)
        self.installButton.setMinimumSize(QtCore.QSize(0,50))
        self.installButton.setIcon(QtGui.QIcon(":images/bottom.png"))
        self.installButton.setIconSize(QtCore.QSize(32,32))
        self.verticalLayout.addWidget(self.installButton)
        spacerItem2 = QtGui.QSpacerItem(20,40,QtGui.QSizePolicy.Minimum,QtGui.QSizePolicy.Expanding)
        self.verticalLayout.addItem(spacerItem2)
        self.upgradeButton = QtGui.QPushButton(self)
        self.upgradeButton.setMinimumSize(QtCore.QSize(0,50))
        self.upgradeButton.setIcon(QtGui.QIcon(":images/tool.png"))
        self.upgradeButton.setIconSize(QtCore.QSize(32,32))
        self.verticalLayout.addWidget(self.upgradeButton)
        spacerItem2 = QtGui.QSpacerItem(20,40,QtGui.QSizePolicy.Minimum,QtGui.QSizePolicy.Expanding)
        self.verticalLayout.addItem(spacerItem2)
        self.rebuildButton = QtGui.QPushButton(self)
        self.rebuildButton.setMinimumSize(QtCore.QSize(0,50))
        self.rebuildButton.setIcon(QtGui.QIcon(":images/reload.png"))
        self.rebuildButton.setIconSize(QtCore.QSize(32,32))
        self.verticalLayout.addWidget(self.rebuildButton)
        self.gridLayout.addLayout(self.verticalLayout,1,1,1,1)
        spacerItem3 = QtGui.QSpacerItem(20,40,QtGui.QSizePolicy.Minimum,QtGui.QSizePolicy.Expanding)
        self.gridLayout.addItem(spacerItem3,3,1,1,1)
        spacerItem4 = QtGui.QSpacerItem(40,20,QtGui.QSizePolicy.Expanding,QtGui.QSizePolicy.Minimum)
        self.gridLayout.addItem(spacerItem4,1,2,1,1)
        self.retranslateUi()
        self.connectAll()
#-----------------------------------------------------------
    def retranslateUi(self):
        '''Set all the texts.'''
        self.installButton.setText(self.tr("   Install ILDJIT"))
        self.upgradeButton.setText(self.tr("   Upgrade ILDJIT"))
        self.rebuildButton.setText(self.tr("   Rebuild ILDJIT"))
#-----------------------------------------------------------
    def install(self):
        '''Start the installation'''
        self.parent.action=self.parent.INSTALL
        self.parent.moveTo(1)
#-----------------------------------------------------------
    def upgrade(self):
        '''Start the upgrade'''
        self.parent.action=self.parent.UPGRADE
        self.parent.moveTo(1)
#-----------------------------------------------------------
    def rebuild(self):
        '''Start the rebuild'''
        self.parent.action=self.parent.REBUILD
        self.parent.moveTo(1)
#-----------------------------------------------------------
    def connectAll(self):
        '''Connect all the signals'''
        self.connect(self.installButton, QtCore.SIGNAL("clicked()"),self.install)
        self.connect(self.upgradeButton, QtCore.SIGNAL("clicked()"),self.upgrade)
        self.connect(self.rebuildButton, QtCore.SIGNAL("clicked()"),self.rebuild)
#-----------------------------------------------------------
    def showEvent(self,event):
        self.setVisible(True)