from PyQt4 import QtCore, QtGui
class ClickLineEdit(QtGui.QLineEdit):
    '''Reimplementation of the QLineEdit class.
    Done in order to catch a click event by the user.'''
    def __init__(self,parent,enabled=True):
        QtGui.QLineEdit.__init__(self)
        self.parent=parent
        self.setReadOnly(True)
        self.enabled=enabled
    def mousePressEvent (self, event):
        '''Catch the click by the user.'''
        if self.enabled ==True:
            name = QtGui.QFileDialog.getExistingDirectory(self.parent,"Open Directory")
            if not name == "":
                self.setText(name)
                

            
