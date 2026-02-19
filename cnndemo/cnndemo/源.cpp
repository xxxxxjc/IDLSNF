#include <Windows.h>
#include <iostream>

#include <Python.h>

int main()
{
    //ShowWindow(GetConsoleWindow(), SW_HIDE);
    Py_SetPythonHome(L"C:/Users/zhangxiang/anaconda3/envs/xjc38");
    //Py_SetPath(L"F:/Anaconda/envs/xjc38/Lib/site-packages/;""F:/Anaconda/envs/xjc38/DLLs/;""F:/Anaconda/envs/xjc38/;""F:/Anaconda/envs/xjc38/Lib/;""F:/Anaconda/envs/xjc38/Library/bin/;""F:/Anaconda/envs/xjc38/scripts/;""F:/Anaconda/envs/xjc38/Lib/site-packages/torch/lib;");
    Py_Initialize();
    if (!Py_IsInitialized())
    {
        printf("Initialized failed");
    }
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('E:/xjc/test/DLSN-main/DLSN-main/src')");
    PyObject* pModule = PyImport_ImportModule("cnn");
    PyObject* pFunc = PyObject_GetAttrString(pModule,"main");
    PyObject* pReturnValue = PyObject_CallObject(pFunc, NULL);
}