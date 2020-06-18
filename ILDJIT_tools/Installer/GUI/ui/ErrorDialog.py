from PyQt4 import QtCore, QtGui
class ErrorDialog(QtGui.QDialog):
    '''This dialog show an error message to notify the user a failure during the installation.'''
    def __init__(self,parent,title,header,text):
        '''@param parent: the parent widget of the dialog
        @param title: the title of the dialog
        @param header: the main message to be shown to the user
        @param text: the text of the failure'''
        QtGui.QDialog.__init__(self,parent)
        self.parent=parent
        self.header=header
        self.text=text
        self.title=title
        self.setupUi()
    def setupUi(self):
        self.resize(400, 300)
        self.gridLayout = QtGui.QGridLayout(self)
        self.headerLabel = QtGui.QLabel(self)
        self.gridLayout.addWidget(self.headerLabel, 1, 0, 1, 1)
        self.textEdit = QtGui.QTextEdit(self)
        self.textEdit.setReadOnly(True)
        self.gridLayout.addWidget(self.textEdit, 3, 0, 1, 1)
        spacerItem = QtGui.QSpacerItem(20, 30, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Fixed)
        self.gridLayout.addItem(spacerItem, 0, 0, 1, 1)
        spacerItem1 = QtGui.QSpacerItem(20, 15, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Fixed)
        self.gridLayout.addItem(spacerItem1, 2, 0, 1, 1)
        self.horizontalLayout = QtGui.QHBoxLayout()
        spacerItem2 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem2)
        self.okButton = QtGui.QPushButton(self)
        self.okButton.setMinimumSize(QtCore.QSize(0, 30))
        self.okButton.setText("Ok")
        self.horizontalLayout.addWidget(self.okButton)
        self.gridLayout.addLayout(self.horizontalLayout, 5, 0, 1, 1)
        self.textEdit.setText(self.text)
        self.setWindowTitle(self.title)
        self.headerLabel.setText(self.header)
        self.connectAll()
    def connectAll(self):
        '''Connect all the signals'''
        self.connect(self.okButton, QtCore.SIGNAL("clicked()"),self.accept)