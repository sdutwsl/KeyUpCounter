#!/usr/bin/python
# coding=utf-8
import requests
from bs4 import BeautifulSoup

res = requests.get("https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes")


soup1 = BeautifulSoup(res.content, 'html.parser')

soup2 = soup1.find('tbody')
trs = soup2.find_all('tr')
str="["
for tr in trs:
    str+='\t{'
    tds = tr.find_all('td')
    dts = tds[0].find_all('dt')
    str+='\t\t"shot_name":"'+dts[0].text+'",'
    str+='\t\t"value":"'+dts[1].text+'",'
    str+='\t\t"meaning":"'+tds[1].text+'"'
    str+='\t}'+['',','][dts[0].text != "VK_OEM_CLEAR"]
str+=']'

f=open('key_maps.json','w')
f.write(str)
f.close()