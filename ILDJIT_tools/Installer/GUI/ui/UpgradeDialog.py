from PyQt4 import QtCore, QtGui
import sys
class UpgradeDialog(QtGui.QDialog):
    '''Show the user the packages to be upgraded.'''
#-----------------------------------------------------------
    def __init__(self,parent,list):
        '''@param parent: the parent widget of the dialog
        @param list: the list of the packages to be displayed'''
        QtGui.QDialog.__init__(self,parent)
        self.setupUi()
        self.setText(list)
#-----------------------------------------------------------
    def setupUi(self):
        self.resize(400,300)
        self.gridLayout = QtGui.QGridLayout(self)
        self.listWidget = QtGui.QListWidget(self)
        applications = QtCore.QStringList()
        self.gridLayout.addWidget(self.listWidget,1,0,1,1)
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.upgradeButton = QtGui.QPushButton(self)
        self.upgradeButton.setMinimumSize(QtCore.QSize(0,50))
        self.horizontalLayout.addWidget(self.upgradeButton)
        spacerItem = QtGui.QSpacerItem(40,20,QtGui.QSizePolicy.Expanding,QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.cancelButton = QtGui.QPushButton(self)
        self.cancelButton.setMinimumSize(QtCore.QSize(0,50))
        self.horizontalLayout.addWidget(self.cancelButton)
        self.gridLayout.addLayout(self.horizontalLayout,2,0,1,1)
        self.titleLabel = QtGui.QLabel(self)
        self.gridLayout.addWidget(self.titleLabel,0,0,1,1)
        self.retranslateUi()
        self.connectAll()
#-----------------------------------------------------------
    def retranslateUi(self):
        '''Set all the texts.'''
        self.upgradeButton.setText(self.tr("Upgrade"))
        self.cancelButton.setText(self.tr("Cancel"))
#-----------------------------------------------------------
    def connectAll(self):
        '''Connect all the signals'''
        self.connect(self.cancelButton, QtCore.SIGNAL("clicked()"),self.reject)
        self.connect(self.upgradeButton, QtCore.SIGNAL("clicked()"),self.accept)
#-----------------------------------------------------------
    def setText(self,list):
        '''Set the message to be displayed to the user.'''
        if len(list) ==0:
            self.titleLabel.setText(self.tr("No Updates Found!"))
            self.upgradeButton.setEnabled(False)
        else:
            self.titleLabel.setText(self.tr("Updates Found!\nUpdates:"))
            self.listWidget.insertItems(0, list)