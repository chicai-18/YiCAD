### 支持Python 3.12版本 ###
### 参考 https://www.jb51.net/article/184066.htm

import os, sys
import chardet
from charset_normalizer import detect

# 需要生成的文件编码
tartget_coding = 'UTF-8-SIG'    # utf8带BOM
# tartget_coding = 'utf-8'

# 忽略的文件
except_files = ['exp1.txt','exp2.txt']

# 仅修改指定拓展名的文件，为空不限定拓展名
filter_exts = ['.txt', '.cpp', '.h']
# filter_exts = []
# 仅修改指定文件夹，当前py文件目录下的子目录，为空作用于所有文件夹
filter_folders = ['libraries', 'plugins', 'YiCAD']

# 可信度设置
confidence = 0.98

# 转换失败的文件
parse_fail_files = []

# 获得所有满足得文件
def find_all_files(path: str) -> str:
    for root, dirs, files in os.walk(path):
        # 判断文件夹是否满足
        folder_valid = False
        if len(filter_folders) == 0:
            folder_valid = True
        else:
            subStr = root[len(path) + 1:len(root)]
            sepIdx = subStr.find(os.sep)
            if sepIdx != -1:
                subStr = subStr[0:sepIdx]
            for folder in filter_folders:
                if folder == subStr:
                    folder_valid = True
                    break
        if folder_valid == False:
            continue

        # 判断文件拓展名，文件名是否满足
        for f in files:
            file_valid = False
            if len(filter_exts) == 0:
                file_valid = True
            else:
                for ext in filter_exts:
                    if f.endswith(ext):
                        file_valid = True
                        break
            # 文件名是否有效
            if file_valid:
                for exc_file in except_files:
                    if exc_file == f:
                        file_valid = False
                        break
            if file_valid:
                fullname = os.path.join(root, f)
                yield fullname
        pass
    pass


# 判断是不是utf-8编码方式
def judge_coding(path: str) -> dict:
    with open(path, 'rb') as f:
        # c = detect(f.read())
        c = chardet.detect(f.read())
    if c['encoding'] != tartget_coding:
        return c

# 修改文件编码方式
def change_to_utf_file(path: str):
    for i in find_all_files(path):
        c = judge_coding(i)
        if c:
            conf = c['confidence']
            if conf > confidence :
                change(i, c['encoding'])
                print("{}编码方式已从{}改为{},可信度{}".format(i, c['encoding'], tartget_coding, conf))
            else:
                err = "{},可信度{}".format(i, conf)
                parse_fail_files.append(err)
    if len(parse_fail_files) != 0:
        print("存在转换失败文件：")
        print(*parse_fail_files,sep = "\n")
# 将单个文件转换为指定编码
def change(path: str, coding: str):
    try:
        with open(path, 'r', encoding=coding) as f:
            text = f.read()
        with open(path, 'w', encoding=tartget_coding) as f:
            f.write(text)
    except:
        parse_fail_files.append(path)

# 查看所有文件编码方式
def check(path: str):
    for i in find_all_files(path):
        with open(i, 'rb') as f:
            print(chardet.detect(f.read())['encoding'], ': ', i)
def main():
    my_path= '.'
    #check(my_path)
    change_to_utf_file(my_path)
    input("回车以关闭")
if __name__ == '__main__':
    main()

