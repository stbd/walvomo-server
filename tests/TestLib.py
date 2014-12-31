import time
import _mysql

def validateThatPageHasText(source, text):
    if source.find(text) == -1:
        raise Exception('Text was not found')

def findSelectInputFieldWithDefaultText(pageContent, text, prefixText = None):
    
    prefixIndex = 0
    if prefixText != None:
        prefixIndex = pageContent.find(prefixText)
    
    textStart = pageContent[prefixIndex:].find(text)
    if textStart == -1:
        raise Exception("textStart not found")
        
    textStart = textStart + prefixIndex
    inputStart = pageContent[0:textStart].rfind('<select')
    if inputStart == -1:
        raise Exception("Input start not found")
    
    idTag = 'id="'
    idStart = pageContent[inputStart:].find(idTag)
    index = inputStart + idStart + len(idTag)
    idLength = pageContent[index:].find('"')
    print pageContent[index:index + idLength]
    return pageContent[index:index + idLength]
    

def findInputFieldWithTextBeforeIt(pageContent, text, prefixText = None, inputType = 'input'):

    prefixIndex = 0
    if prefixText != None:
        prefixIndex = pageContent.find(prefixText)

    textStart = pageContent[prefixIndex:].find(text)
    textStart = textStart + prefixIndex
    inputStart = pageContent[textStart:].find(inputType)
    
    if textStart == -1 or inputStart == -1:
        raise Exception("Input field id not found")
    
    idTag = 'id="'
    idStart = pageContent[textStart + inputStart:].find(idTag)
    index = textStart + inputStart + idStart + len(idTag)
    idLength = pageContent[index:].find('"')
    print pageContent[index:index + idLength]
    return pageContent[index:index + idLength]


def findButtonWithText(pageContent, buttonText, prefixText = None):
    
    prefixIndex = 0
    if prefixText != None:
        prefixIndex = pageContent.find(prefixText)    
    
    idTag = 'id="'
    textStart = pageContent[prefixIndex:].find(buttonText)
    textStart = textStart + prefixIndex
    idStart = pageContent.rfind(idTag, 0, textStart)
    idStart = idStart + len(idTag)
    if textStart == -1 or idStart == -1:
        raise Exception("Button not found")
    
    idLength = pageContent[idStart:].find('"')
    return pageContent[idStart:idStart+idLength]

def findLinkWithText(pageContent, elementText):
    
    idTag = 'id="'
    textStart = pageContent.find(elementText)
    idStart = pageContent.rfind(idTag, 0, textStart)
    idStart = idStart + len(idTag)
    if textStart == -1 or idStart == -1:
        raise Exception("Element not found")
    
    idLength = pageContent[idStart:].find('"')
    return pageContent[idStart:idStart+idLength]

def testIfStringIsFound(pageContent, str):
    print pageContent
    if pageContent.find(str) == -1:
        raise Exception("Text not found")
    
def wait(sleepInSeconds):
    time.sleep(float(sleepInSeconds))
    
def emptyFile(filename):
    f = open(filename, 'w')
    f.close();
    
def validateEmailFileAndGetValidationAddres(filename, username):
    d = ''
    tries = 0
    while tries < 5:
        f = open(filename, 'r')
        d = f.read()
        f.close()
        if len(d) < 5:
            d = ''
            time.sleep(1.0)
            tries = tries + 1
        else:
            break
    
    if d.find(username) == -1:
        raise Exception("Username not found in verification mail")
    
    addressStart = d.find('http://')
    if addressStart == -1:
        raise Exception("Verification address not found in verification mail")
    addressEnd = min(d[addressStart:].find('\n'), d[addressStart:].find(' '))
    addressEnd = addressEnd + addressStart
    address = d[addressStart:addressEnd]
    print address
    return address

def clearUsersFromPersistentUserDb(username):
    con = _mysql.connect('localhost', 'wDb', 'sfhJtBZt', 'w')
    #con = _mysql.connect('localhost', 'ocdb', 'OcapP', 'oc')
    deleteCmd = 'DELETE FROM user WHERE username=\"' + username + '\";'
    con.query(deleteCmd)

def verityThatUserDoesNotExistInPersistantStorage(username):
    wait(1)                                                     #Remove if better solution is found
    con = _mysql.connect('localhost', 'wDb', 'sfhJtBZt', 'w')
    #con = _mysql.connect('localhost', 'ocdb', 'OcapP', 'oc')
    selectCmd = 'SELECT * FROM user WHERE username=\"' + username + '\";'
    con.query(selectCmd)
    res = con.use_result()
    if res == None:
        raise Exception('Select failed')
    t = res.fetch_row(maxrows = 0)
    print t
    if len(t) != 0:
        raise Exception('User was found in persistent storage')
        
def verifyThatUserExistsInPersistentStorage(username, gender, email, yearOfBirth):
    wait(1)                                                     #Remove if better solution is found
    con = _mysql.connect('localhost', 'wDb', 'sfhJtBZt', 'w')
    #con = _mysql.connect('localhost', 'ocdb', 'OcapP', 'oc')
    selectCmd = 'SELECT * FROM user WHERE username=\"' + username + '\";'
    con.query(selectCmd)
    res = con.use_result()
    if res == None:
        raise Exception('Select failed')
    
    t = res.fetch_row(maxrows = 0)
    if len(t) != 1:
        raise Exception('Multiple users was not found in persistent storage')
    t = t[0]
    if t[2] != username or t[3].lower() != gender.lower() or str(t[4]) != str(email) or str(t[5]) != str(yearOfBirth):
        raise Exception('User was not found in persistent storage') 
        
            
    