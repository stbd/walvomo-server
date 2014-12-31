wget http://robotframework.googlecode.com/files/robotframework-2.7.3.tar.gz
wget http://robotframework-seleniumlibrary.googlecode.com/files/robotframework-seleniumlibrary-2.9.tar.gz

tar xfv robotframework-2.7.3.tar.gz
tar xfv robotframework-seleniumlibrary-2.9.tar.gz

cd robotframework-2.7.3
python setup.py build
sudo python setup.py install
cd ..

cd robotframework-seleniumlibrary-2.9
python setup.py build
sudo python setup.py install
cd ..

rm robotframework-seleniumlibrary-2.9 robotframework-2.7.3 robotframework-2.7.3.tar.gz robotframework-seleniumlibrary-2.9.tar.gz -Rf

#Create test user
sudo adduser testuser
mail -s Test testuser@localhost < /dev/null
sudo chmod 777 /var/mail/testuser

sudo adduser testuserpersistent
mail -s Test testuserpersistent@localhost < /dev/null
sudo chmod 777 /var/mail/testuserpersistent
