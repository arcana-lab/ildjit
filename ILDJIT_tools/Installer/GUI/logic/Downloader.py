from PyQt4 import QtCore
import urllib
class Downloader(QtCore.QThread):
    '''This class is used to download a package.
    The progress in the download will be notified to the class "DownTableWidget".'''
#--------------------------------------------------------------
    def __init__(self,url,file,index,folder):
        '''@param url: the base url,
        @param file: the file to be downloded,
        @param index: the number of the progress bar,
        @param folder: the directory where to put the downloaded file'''
        QtCore.QThread.__init__(self)
        self.url=url
        self.file=file
        self.index=index
        self.folder=folder
#--------------------------------------------------------------
    def run(self):
        '''Perform the download'''
        try:
            urllib.urlretrieve(self.url, self.folder+"/"+self.file, reporthook=self.emitter)
        except:
            self.emit(QtCore.SIGNAL("downloadingError"),self.file,self.url)
#--------------------------------------------------------------
    def emitter(self,count, blockSize, totalSize):
        '''Emit a signal to the progress bar
        @param count: the number of blocks transferred so far
        @param blockSize: block size in bytes
        @param totalSize: the total size of the file'''
        if totalSize == 0:
            self.emit(QtCore.SIGNAL("downloadingError"),self.file,self.url)
        else:
            percent = int(count*blockSize*100/totalSize)
            if percent > 100 :
                percent =100
            self.emit(QtCore.SIGNAL("setValue"),self.index, percent)
            if percent ==100:
                self.emit(QtCore.SIGNAL("downloadCompleted"))
