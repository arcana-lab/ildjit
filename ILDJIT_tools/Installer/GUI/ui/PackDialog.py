from PyQt4 import QtCore, QtGui
from config.ReqPackParser import ReqPackParser
class PackDialog(QtGui.QDialog):
    '''This dialog displays the required software required to install ildjit.'''
#-----------------------------------------------------------
    def __init__(self,parent,list):
        QtGui.QDialog.__init__(self,parent)
        self.paret=parent
        self.list=list
        self.setupUi()
#-----------------------------------------------------------
    def setupUi(self):
        self.resize(400, 300)
        self.gridLayout = QtGui.QGridLayout(self)
        self.label = QtGui.QLabel(self)
        self.gridLayout.addWidget(self.label, 0, 0, 1, 1)
        self.label_2 = QtGui.QLabel(self)
        self.gridLayout.addWidget(self.label_2, 3, 0, 1, 1)
        self.listWidget = QtGui.QListWidget(self)
        self.listWidget.insertItems(0, self.list)
        self.gridLayout.addWidget(self.listWidget, 2, 0, 1, 1)
        self.horizontalLayout = QtGui.QHBoxLayout()
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.okButton = QtGui.QPushButton(self)
        self.okButton.setMinimumSize(QtCore.QSize(0, 50))
        self.horizontalLayout.addWidget(self.okButton)
        self.gridLayout.addLayout(self.horizontalLayout, 7, 0, 1, 1)
        self.retranslateUi()
        self.connectAll()
#-----------------------------------------------------------
    def connectAll(self):
        '''Connect all the signals'''
        self.connect(self.okButton, QtCore.SIGNAL("clicked()"),self.close)
#-----------------------------------------------------------
    def retranslateUi(self):
        '''Set all the texts.'''
        self.setWindowTitle(self.tr("Required Packages"))
        self.label.setText(self.tr("Required Packages:"))
        self.label_2.setText(self.tr("On Debian-like distributions try <b>apt-get install...<b>"))
        self.okButton.setText(self.tr("Ok"))