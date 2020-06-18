from PyQt4 import QtCore, QtGui
class LastWid(QtGui.QWidget):
    '''This widget show a success message.'''
#-----------------------------------------------------------
    def __init__(self,parent=None):
        QtGui.QWidget.__init__(self,parent) 
        self.parent=parent
        self.width=500
        self.height=400
        self.hide()
        self.setupUi()
#-----------------------------------------------------------
    def setupUi(self):
        self.resize(400, 300)
        self.gridLayout = QtGui.QGridLayout(self)
        self.verticalLayout = QtGui.QVBoxLayout()
        self.label = QtGui.QLabel(self)
        self.verticalLayout.addWidget(self.label)
        self.horizontalLayout_2 = QtGui.QHBoxLayout()
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_2.addItem(spacerItem)
        self.label_2 = QtGui.QLabel(self)
        self.label_2.setPixmap(QtGui.QPixmap(":images/ok.png"))
        self.horizontalLayout_2.addWidget(self.label_2)
        spacerItem1 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_2.addItem(spacerItem1)
        self.verticalLayout.addLayout(self.horizontalLayout_2)
        self.label_3 = QtGui.QLabel(self)
        self.verticalLayout.addWidget(self.label_3)
        self.gridLayout.addLayout(self.verticalLayout, 0, 0, 1, 1)
        self.horizontalLayout = QtGui.QHBoxLayout()
        spacerItem2 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem2)
        self.finishButton = QtGui.QPushButton(self)
        self.finishButton.setMinimumSize(QtCore.QSize(0, 50))
        self.horizontalLayout.addWidget(self.finishButton)
        self.gridLayout.addLayout(self.horizontalLayout, 1, 0, 1, 1)
        self.retranslateUi()
        self.connectAll()
#-----------------------------------------------------------
    def connectAll(self):
        '''Connect all the signals'''
        self.connect(self.finishButton, QtCore.SIGNAL("clicked()"),self.parent.close)
#-----------------------------------------------------------
    def retranslateUi(self):
        '''Set all the texts.'''
        self.label_3.setText(self.tr("Have Fun !"))
        self.finishButton.setText(self.tr("Finish"))
#----------------------------------------------------------
    def showEvent(self,event):
        '''Action performed when the widget is shown to the user.'''
        if self.parent.action == self.parent.INSTALL:
            self.label.setText(self.tr("<center><font size=\"7\"><b>Installation Completed</b></font></center>"))
        elif self.parent.action == self.parent.UPGRADE:
            self.label.setText(self.tr("<center><font size=\"7\"><b></b>Upgrade Completed</font></center>"))
        elif self.parent.action == self.parent.REBUILD:
            self.label.setText(self.tr("<center><font size=\"7\"><b>Rebuild Completed</b></font></center>"))
#-----------------------------------------------------------
    def showEvent(self,event):
        self.setVisible(True)