from PyQt4 import QtCore, QtGui
from subprocess import *
class BashWid(QtGui.QWidget):
    '''This widget asks the user if he wants to upgrade his .bashrc file'''
#-----------------------------------------------------------
    def __init__(self,parent=None):
        QtGui.QWidget.__init__(self,parent) 
        self.parent=parent
        self.width=600
        self.height=400
        self.hide()
        self.setupUi()
#-----------------------------------------------------------
    def setupUi(self):
        self.setMinimumSize(QtCore.QSize(self.width,self.height))
        self.gridLayout = QtGui.QGridLayout(self)
        self.label = QtGui.QLabel(self)
        self.gridLayout.addWidget(self.label, 0, 0, 1, 1)
        self.textEdit = QtGui.QTextEdit(self)
        self.textEdit.setReadOnly(True)
        self.gridLayout.addWidget(self.textEdit, 3, 0, 1, 1)
        self.label_2 = QtGui.QLabel(self)
        self.gridLayout.addWidget(self.label_2, 1, 0, 1, 1)
        self.horizontalLayout = QtGui.QHBoxLayout()
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.okButton = QtGui.QPushButton(self)
        self.okButton.setMinimumSize(QtCore.QSize(0, 50))
        self.horizontalLayout.addWidget(self.okButton)
        self.cancelButton = QtGui.QPushButton(self)
        self.cancelButton.setMinimumSize(QtCore.QSize(0, 50))
        self.horizontalLayout.addWidget(self.cancelButton)
        self.gridLayout.addLayout(self.horizontalLayout, 5, 0, 1, 1)
        self.label_3 = QtGui.QLabel(self)
        self.gridLayout.addWidget(self.label_3, 2, 0, 1, 1)
        self.retranslateUi()
        self.connectAll()
        self.performed=False
#----------------------------------------------------------
    def retranslateUi(self):
        '''Set all the texts.'''
        self.label.setText(self.tr("Would you like to upgrade your .bashrc file ?"))
        self.label_2.setText(self.tr("This is strongly recommended in order to set the correct environment variables!"))
        self.okButton.setText(self.tr("Upgrade"))
        self.cancelButton.setText(self.tr("Cancel"))
        self.label_3.setText(self.tr("This will be the result (new lines are in red):"))
#----------------------------------------------------------
    def connectAll(self):
        '''Connect all the signals'''
        self.connect(self.okButton, QtCore.SIGNAL("clicked()"),self.write)
        self.connect(self.cancelButton, QtCore.SIGNAL("clicked()"),self.next)
#----------------------------------------------------------
    def next(self):
        '''Move to the next widget.'''
        self.parent.moveTo(1)
#----------------------------------------------------------
    def showEvent(self,event):
        '''Action performed when the widget is shown.
        The .bashrc file is read and shown to the user.'''
        if not event.spontaneous() and not self.performed:
            self.performed=True
            temp=Popen('echo $HOME/.bashrc',shell=True,stdout=PIPE,stderr=PIPE).communicate()[0]
            self.file_path=temp[:len(temp)-1]
            file=None
            try:
                file = open(self.file_path,'r')
            except:        
                if not self.isVisible():
                    self.okButton.setEnabled(False)
                    self.setVisible(True)
                QtGui.QMessageBox.critical(self,"Error while reading the file", "Error while reading "+self.file_path+".");
            else:
                for line in file:
                    self.textEdit.append(line)
                file.close()
                self.strings=[]
                self.strings.append("export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"+self.parent.installDir+":"+self.parent.installDir+"/lib:"+self.parent.installDir+"/lib/iljit")
                self.strings.append("export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:"+self.parent.installDir+"/lib/pkgconfig")
                self.strings.append("export PATH=$PATH:"+self.parent.installDir+"/bin")
                self.strings.append("export ILDJIT_PATH="+self.parent.pnetDir+"/lib/cscc/lib")
                self.textEdit.append("\n<font color=\"red\">"+"<br>"+"<br>".join(self.strings)+"<br>"+"</font>")
        if not self.isVisible():
            self.setVisible(True)
#----------------------------------------------------------
    def write(self): 
        '''Upgrade the file with the new lines.'''
        file=None
        try:
            file = open(self.file_path,'a')
        except:
            self.okButton.setEnabled(False)
            QtGui.QMessageBox.critical(self,"Error while reading the file", "Error while reading "+self.file_path+".");
        else:
            file.write("\n"+"\n".join(self.strings)+"\n")
            file.close()
            self.next()
