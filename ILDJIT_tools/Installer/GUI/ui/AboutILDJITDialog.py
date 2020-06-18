from PyQt4 import QtCore, QtGui
class AboutILDJITDialog(QtGui.QDialog):
    '''Dialog window with info about ildjit'''
    def __init__(self,parent):
        QtGui.QDialog.__init__(self,parent);
        self.setupUi()
    def setupUi(self):
        self.resize(458, 239)
        self.gridLayout = QtGui.QGridLayout(self)
        self.vboxlayout = QtGui.QVBoxLayout()
        self.label_2 = QtGui.QLabel(self)
        self.vboxlayout.addWidget(self.label_2)
        self.label_3 = QtGui.QLabel(self)
        self.vboxlayout.addWidget(self.label_3)
        self.gridLayout.addLayout(self.vboxlayout, 0, 0, 1, 1)
        self.gridlayout = QtGui.QGridLayout()
        self.gridlayout.setContentsMargins(-1, -1, 0, -1)
        self.gridlayout.setHorizontalSpacing(0)
        self.gridlayout.setVerticalSpacing(6)
        self.label_6 = QtGui.QLabel(self)
        self.gridlayout.addWidget(self.label_6, 0, 0, 1, 1)
        self.label_7 = QtGui.QLabel(self)
        self.gridlayout.addWidget(self.label_7, 0, 1, 1, 1)
        self.label = QtGui.QLabel(self)
        self.gridlayout.addWidget(self.label, 1, 1, 1, 1)
        self.label_5 = QtGui.QLabel(self)
        self.gridlayout.addWidget(self.label_5, 1, 0, 1, 1)
        self.label_4 = QtGui.QLabel(self)
        self.gridlayout.addWidget(self.label_4, 2, 0, 1, 1)
        self.label_8 = QtGui.QLabel(self)
        self.gridlayout.addWidget(self.label_8, 2, 1, 1, 1)
        self.gridLayout.addLayout(self.gridlayout, 1, 0, 1, 1)
        self.hboxlayout = QtGui.QHBoxLayout()
        spacerItem = QtGui.QSpacerItem(121, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.hboxlayout.addItem(spacerItem)
        self.okButton = QtGui.QPushButton(self)
        self.okButton.setMinimumSize(QtCore.QSize(0, 50))
        self.hboxlayout.addWidget(self.okButton)
        self.gridLayout.addLayout(self.hboxlayout, 2, 0, 1, 1)
        self.retranslateUi()
        self.connectAll()
    def retranslateUi(self):
        '''Set all the texts.'''
        self.setWindowTitle(self.tr("About ILDJIT"))
        self.label_2.setText(self.tr("<font size=\"7\"><b>ILDJIT</b></font>"))
        self.label_3.setText(self.tr("<font size=\"4\"><b>Intermediate Language Distributed Just In Time</b></font>"))
        self.label_6.setText(self.tr("<font size=\"3\"><b>Author:</b></font>"))
        self.label_7.setText(self.tr("Simone Campanoni"))
        self.label.setText(self.tr("simo.xan@gmail.com"))
        self.label_5.setText(self.tr("<font size=\"3\"><b>Email:</b></font>"))
        self.label_4.setText(self.tr("<font size=\"3\"><b>Site:</b></font>"))
        self.label_8.setText(self.tr("http://ildjit.sourceforge.net/"))
        self.okButton.setText(self.tr("Ok"))
#----------------------------------------------------------
    def connectAll(self):
        '''Connect all the signals'''
        self.connect(self.okButton, QtCore.SIGNAL("clicked()"), self.close)