#!/usr/python

import os,sys
import string

fileListInDir = os.listdir("./")

for fileName in fileListInDir:
	os.rename(fileName,string.replace(fileName,'_','-'))
os.system("find ./ -name '*.po' -exec perl -pi -e 's/IDS_BR_HEADER_ERROR_PAGE/Error Page/g' {} \;")
os.system("find ./ -name '*.po' -exec perl -pi -e 's/IDS_BR_BODY_WEB_PAGE_TEMPORARILY_NOT_AVAILABLE/Web page temporarily not available/g' {} \;")
os.system("find ./ -name '*.po' -exec perl -pi -e 's/IDS_BR_BODY_WEB_PAGE_NOT_AVAILABLE/Web page not available/g' {} \;")
os.system("find ./ -name '*.po' -exec perl -pi -e 's/IDS_BR_BODY_FAILED_TO_LOAD_FRAMES/Failed to load frames/g' {} \;")
os.system("find ./ -name '*.po' -exec perl -pi -e 's/IDS_BR_BODY_LOAD_FAILED_MSG_1/While retrieving Web page %s, the following error occurred./g' {} \;")
os.system("find ./ -name '*.po' -exec perl -pi -e 's/IDS_BR_BODY_LOAD_FAILED_MSG_2/Unable to retrieve Web page. (Web page might be temporarily down or have moved to new URL)/g' {} \;")
os.system("find ./ -name '*.po' -exec perl -pi -e 's/IDS_BR_BODY_LOAD_FAILED_MSG_3/The most likely cause is given below/g' {} \;")
os.system("find ./ -name '*.po' -exec perl -pi -e 's/IDS_BR_BODY_LOAD_FAILED_MSG_4/Network connection not established normally/g' {} \;")
os.system("find ./ -name '*.po' -exec perl -pi -e 's/IDS_BR_BODY_LOAD_FAILED_MSG_5/Check Web page URL/g' {} \;")
os.system("find ./ -name '*.po' -exec perl -pi -e 's/IDS_BR_BODY_LOAD_FAILED_MSG_6/Reload Web page later/g' {} \;")
os.system("find ./ -name '*.po' -exec perl -pi -e 's/\"%s\"/%s/g' {} \;")
