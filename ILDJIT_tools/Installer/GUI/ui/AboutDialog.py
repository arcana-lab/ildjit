from PyQt4 import QtCore, QtGui
class AboutDialog(QtGui.QDialog):
    '''Dialog window with info about the installer'''
    def __init__(self,parent):
        QtGui.QDialog.__init__(self,parent);
        self.setupUi()
    def setupUi(self):
        self.resize(458, 220)
        self.gridlayout = QtGui.QGridLayout(self)
        self.gridlayout.setSpacing(0)
        self.gridlayout.setContentsMargins(-1, -1, 9, -1)
        self.vboxlayout = QtGui.QVBoxLayout()
        self.label_2 = QtGui.QLabel(self)
        self.vboxlayout.addWidget(self.label_2)
        self.label_3 = QtGui.QLabel(self)
        self.vboxlayout.addWidget(self.label_3)
        self.gridlayout.addLayout(self.vboxlayout, 0, 0, 1, 1)
        self.gridlayout1 = QtGui.QGridLayout()
        self.gridlayout1.setContentsMargins(-1, -1, 0, -1)
        self.gridlayout1.setHorizontalSpacing(0)
        self.gridlayout1.setVerticalSpacing(6)
        self.label_6 = QtGui.QLabel(self)
        self.gridlayout1.addWidget(self.label_6, 0, 0, 1, 1)
        self.label_7 = QtGui.QLabel(self)
        self.gridlayout1.addWidget(self.label_7, 0, 1, 1, 1)
        self.label_5 = QtGui.QLabel(self)
        self.gridlayout1.addWidget(self.label_5, 1, 0, 1, 1)
        self.label_4 = QtGui.QLabel(self)
        self.gridlayout1.addWidget(self.label_4, 1, 1, 1, 1)
        self.label_8 = QtGui.QLabel(self)
        self.gridlayout1.addWidget(self.label_8, 2, 0, 1, 1)
        self.label = QtGui.QLabel(self)
        self.gridlayout1.addWidget(self.label, 2, 1, 1, 1)
        self.gridlayout.addLayout(self.gridlayout1, 1, 0, 1, 1)
        self.hboxlayout = QtGui.QHBoxLayout()
        spacerItem = QtGui.QSpacerItem(121, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.hboxlayout.addItem(spacerItem)
        self.okButton = QtGui.QPushButton(self)
        self.okButton.setMinimumSize(QtCore.QSize(0, 50))
        self.hboxlayout.addWidget(self.okButton)
        self.gridlayout.addLayout(self.hboxlayout, 2, 0, 1, 1)
        self.connectAll()
        self.retranslateUi()
#----------------------------------------------------------
    def retranslateUi(self):
        '''Set all the texts.'''
        self.setWindowTitle(self.tr("About The Mighty Installer"))
        self.label_2.setText(self.tr("<font size=\"7\"><b>The Mighty Installer</b></font>"))
        self.label_3.setText(self.tr("<font size=\"4\"><b>ILDJIT Installer</b></font>"))
        self.label_6.setText(self.tr("<font size=\"3\"><b>Author:</b></font>"))
        self.label_7.setText(self.tr("Alberto Magni"))
        self.label_5.setText(self.tr("<font size=\"3\"><b>Version:</b></font>"))
        self.label_4.setText(self.tr("0.0.4"))
        self.okButton.setText(self.tr("Ok"))
        self.label_8.setText(self.tr("<font size=\"3\"><b>Email:</b></font>"))
        self.label.setText(self.tr("alberto.magni86@gmail.com"))
#----------------------------------------------------------
    def connectAll(self):
        '''Connect all the signals'''
        self.connect(self.okButton, QtCore.SIGNAL("clicked()"), self.close)
