# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
#-----------------------------------------------------------
class ProgressDialog(QtGui.QProgressDialog):
    '''This dialog show a busy progress bar used during the construction of the tree.'''
    ERROR=0
    START_DOWN=2
    CHECK=3
    REBUILD=4
    def __init__(self,parent):
        QtGui.QProgressDialog.__init__(self,parent)
        self.parent=parent
        self.setValue(-1)
        self.setModal(1)
        self.setMaximum(0)
        self.setCancelButton(None)
        self.setMinimum(0)
        self.setLabelText(self.tr("Building the Tree..."))
        self.setMinimumWidth(300)
        self.setWindowTitle(self.tr("Building Progress"))
        self.closed=False
    def closeEvent(self,event):
        '''Disable the close event by the user, and perform the right action when the event is not spontaneous.'''
        #chiuso dall'utente
        if event.spontaneous():
            event.ignore()
        #chiuso per errore o successo
        else:
            if self.closed==False:
                if self.action==ProgressDialog.ERROR:
                    self.parent.backButton.setEnabled(True)
                    self.parent.startButton.setEnabled(True)
                    self.parent.sourceLine.enabled=True
                    QtGui.QMessageBox.critical(self.parent,self.tr(self.title),self.tr(self.msg));
                elif self.action==ProgressDialog.START_DOWN:
                    self.parent.tableWidget.startDownload(self.parent.sourceDir)
                    #self.parent.nextButton.setEnabled(True)
                elif self.action==ProgressDialog.CHECK:
                    self.parent.showDialog(self.parent.list)
                elif self.action==ProgressDialog.REBUILD:
                    self.parent.nextButton.setEnabled(True)
                self.closed=True
                self.close()