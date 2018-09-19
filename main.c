#include "block.c"
#include "btree.c"
#include "data.c"
#include "file.c"
#include "hfs.c"
#include "low.c"
#include "medium.c"
#include "memcmp.c"
#include "node.c"
#include "os.c"
#include "record.c"
#include "version.c"
#include "volume.c"

#define PY_SSIZE_T_CLEAN 1
#include <Python.h>

static const char NAME_HFSVOL[] = "hfsvol";
static const char NAME_HFSDIR[] = "hfsdir";
static const char NAME_HFSFILE[] = "hfsfile";

static PyObject *wrap_mount(PyObject *self, PyObject *args)
{
    char *arg_path; int arg_pnum; int arg_flags;
    if(!PyArg_ParseTuple(args, "sii", &arg_path, &arg_pnum, &arg_flags)) return NULL;
    return PyCapsule_New((void *)hfs_mount(arg_path, arg_pnum, arg_flags), NAME_HFSVOL, NULL);
}

static PyObject *wrap_flush(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c;
    if(!PyArg_ParseTuple(args, "O", &arg_vol_c)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_flush(arg_vol)) return NULL;
    return Py_None;
}

static PyObject *wrap_flushall(PyObject *self, PyObject *args)
{
    hfs_flushall();
    return Py_None;
}

static PyObject *wrap_umount(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c;
    if(!PyArg_ParseTuple(args, "O", &arg_vol_c)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_umount(arg_vol)) return NULL;
    return Py_None;
}

static PyObject *wrap_umountall(PyObject *self, PyObject *args)
{
    hfs_umountall();
    return Py_None;
}

static PyObject *wrap_getvol(PyObject *self, PyObject *args)
{
    char *arg_vol;
    if(!PyArg_ParseTuple(args, "y", &arg_vol)) return NULL;
    return PyCapsule_New((void *)hfs_getvol(arg_vol), NAME_HFSVOL, NULL);
}

static PyObject *wrap_setvol(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c;
    if(!PyArg_ParseTuple(args, "O", &arg_vol_c)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    hfs_setvol(arg_vol);
    return Py_None;
}

static PyObject *wrap_vstat(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c;
    hfsvolent ret_volent;
    if(!PyArg_ParseTuple(args, "O", &arg_vol_c)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_vstat(arg_vol, &ret_volent)) return NULL;
    return Py_BuildValue("y#", (char *)(&ret_volent), sizeof(ret_volent));
}

static PyObject *wrap_vsetattr(PyObject *self, PyObject *args) // problems
{
    hfsvol *arg_vol; PyObject *arg_vol_c; hfsvolent *arg_ent; Py_ssize_t arg_ent_len;
    if(!PyArg_ParseTuple(args, "Oy#", &arg_vol_c, &arg_ent, &arg_ent_len)) return NULL;
    if(arg_ent_len != sizeof(*arg_ent)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_vsetattr(arg_vol, arg_ent)) return NULL;
    return Py_None;
}

static PyObject *wrap_chdir(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_chdir(arg_vol, arg_path)) return NULL;
    return Py_None;
}

static PyObject *wrap_getcwd(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c;
    if(!PyArg_ParseTuple(args, "O", &arg_vol_c)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    return Py_BuildValue("l", hfs_getcwd(arg_vol));
}

static PyObject *wrap_setcwd(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; long arg_id;
    if(!PyArg_ParseTuple(args, "Ol", &arg_vol_c, &arg_id)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_setcwd(arg_vol, arg_id)) return NULL;
    return Py_None;
}

static PyObject *wrap_dirinfo(PyObject *self, PyObject *args) // returns name in bytes object!
{
    hfsvol *arg_vol; PyObject *arg_vol_c; unsigned long argret_id;
    char ret_name[32];
    if(!PyArg_ParseTuple(args, "Ok", &arg_vol_c, &argret_id)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_dirinfo(arg_vol, &argret_id, ret_name)) return NULL;
    return Py_BuildValue("ly", argret_id, ret_name);
}

static PyObject *wrap_opendir(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    return PyCapsule_New((void *)hfs_opendir(arg_vol, arg_path), NAME_HFSDIR, NULL);
}

static PyObject *wrap_readdir(PyObject *self, PyObject *args)
{
    hfsdir *arg_dir; PyObject *arg_dir_c;
    hfsdirent ret_ent;
    if(!PyArg_ParseTuple(args, "O", &arg_dir_c)) return NULL;
    if(arg_dir_c == Py_None) arg_dir = NULL;
    else if(!(arg_dir = PyCapsule_GetPointer(arg_dir_c, NAME_HFSDIR))) return NULL;
    if(hfs_readdir(arg_dir, &ret_ent)) return NULL;
    return Py_BuildValue("y#", (char *)(&ret_ent), sizeof(ret_ent));
}

static PyObject *wrap_closedir(PyObject *self, PyObject *args)
{
    hfsdir *arg_dir; PyObject *arg_dir_c;
    if(!PyArg_ParseTuple(args, "O", &arg_dir_c)) return NULL;
    if(arg_dir_c == Py_None) arg_dir = NULL;
    else if(!(arg_dir = PyCapsule_GetPointer(arg_dir_c, NAME_HFSDIR))) return NULL;
    if(hfs_closedir(arg_dir)) return NULL;
    return Py_None;
}

static PyObject *wrap_create(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    char *arg_type; Py_ssize_t arg_type_len;
    char *arg_creator; Py_ssize_t arg_creator_len;
    if(!PyArg_ParseTuple(args, "Oyy#y#", &arg_vol_c, &arg_path, &arg_type, &arg_type_len, &arg_creator, &arg_creator_len)) return NULL;
    if(arg_type_len != 4 || arg_creator_len != 4) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    return PyCapsule_New((void *)hfs_create(arg_vol, arg_path, arg_type, arg_creator), NAME_HFSFILE, NULL);
}

static PyObject *wrap_open(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    return PyCapsule_New((void *)hfs_open(arg_vol, arg_path), NAME_HFSFILE, NULL);
}

static PyObject *wrap_setfork(PyObject *self, PyObject *args)
{
    hfsfile *arg_file; PyObject *arg_file_c; int arg_fork;
    if(!PyArg_ParseTuple(args, "Oi", &arg_file_c, &arg_fork)) return NULL;
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE))) return NULL;
    if(hfs_setfork(arg_file, arg_fork)) return NULL;
    return Py_None;
}

static PyObject *wrap_getfork(PyObject *self, PyObject *args)
{
    hfsfile *arg_file; PyObject *arg_file_c;
    if(!PyArg_ParseTuple(args, "O", &arg_file_c)) return NULL;
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE))) return NULL;
    return Py_BuildValue("i", hfs_getfork(arg_file));
}

static PyObject *wrap_read(PyObject *self, PyObject *args) // pass in a bytearray and get it shrunk!
{
    hfsfile *arg_file; PyObject *arg_file_c; PyObject *arg_bytearray;
    if(!PyArg_ParseTuple(args, "OY", &arg_file_c, &arg_bytearray)) return NULL;
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE))) return NULL;
    long bytesread = hfs_read(arg_file, PyByteArray_AsString(arg_bytearray), PyByteArray_Size(arg_bytearray));
    if(bytesread == -1) return NULL;
    PyByteArray_Resize(arg_bytearray, bytesread);
    return Py_None;
}

static PyObject *wrap_write(PyObject *self, PyObject *args) // pass in a bytearray and get it shrunk!
{
    hfsfile *arg_file; PyObject *arg_file_c; PyObject *arg_bytes; Py_ssize_t arg_bytes_len;
    if(!PyArg_ParseTuple(args, "Oy#", &arg_file_c, &arg_bytes, &arg_bytes_len)) return NULL;
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE))) return NULL;
    long byteswritten = hfs_write(arg_file, arg_bytes, arg_bytes_len);
    if(byteswritten == -1) return NULL;
    return Py_BuildValue("l", byteswritten);
}

static PyObject *wrap_truncate(PyObject *self, PyObject *args) // pass in a bytearray and get it shrunk!
{
    hfsfile *arg_file; PyObject *arg_file_c; unsigned long arg_len;
    if(!PyArg_ParseTuple(args, "Ok", &arg_file_c, &arg_len)) return NULL;
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE))) return NULL;
    if(hfs_truncate(arg_file, arg_len)) return NULL;
    return Py_None;
}

static PyObject *wrap_seek(PyObject *self, PyObject *args) // pass in a bytearray and get it shrunk!
{
    hfsfile *arg_file; PyObject *arg_file_c; long arg_offset; int arg_from;
    if(!PyArg_ParseTuple(args, "Oli", &arg_file_c, &arg_offset, &arg_from)) return NULL;
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE))) return NULL;
    long absloc = hfs_seek(arg_file, arg_offset, arg_from);
    if(absloc == -1) return NULL;
    return Py_BuildValue("l", absloc);
}

static PyObject *wrap_close(PyObject *self, PyObject *args) // pass in a bytearray and get it shrunk!
{
    hfsfile *arg_file; PyObject *arg_file_c;
    if(!PyArg_ParseTuple(args, "O", &arg_file_c)) return NULL;
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE))) return NULL;
    if(hfs_close(arg_file)) return NULL;
    return Py_None;
}

static PyObject *wrap_stat(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    hfsdirent ret_ent;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_stat(arg_vol, arg_path, &ret_ent)) return NULL;
    return Py_BuildValue("y#", (char *)(&ret_ent), sizeof(ret_ent));
}

static PyObject *wrap_fstat(PyObject *self, PyObject *args)
{
    hfsfile *arg_file; PyObject *arg_file_c;
    hfsdirent ret_ent;
    if(!PyArg_ParseTuple(args, "O", &arg_file_c)) return NULL;
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE))) return NULL;
    if(hfs_fstat(arg_file, &ret_ent)) return NULL;
    return Py_BuildValue("y#", (char *)(&ret_ent), sizeof(ret_ent));
}

static PyObject *wrap_setattr(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path; hfsdirent *arg_ent; Py_ssize_t arg_ent_len;
    if(!PyArg_ParseTuple(args, "Oyy#", &arg_vol_c, &arg_path, &arg_ent, &arg_ent_len)) return NULL;
    if(arg_ent_len != sizeof(*arg_ent)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_setattr(arg_vol, arg_path, arg_ent)) return NULL;
    return Py_None;
}

static PyObject *wrap_fsetattr(PyObject *self, PyObject *args)
{
    hfsfile *arg_file; PyObject *arg_file_c; hfsdirent *arg_ent; Py_ssize_t arg_ent_len;
    if(!PyArg_ParseTuple(args, "Oy#", &arg_file_c, &arg_ent, &arg_ent_len)) return NULL;
    if(arg_ent_len != sizeof(*arg_ent)) return NULL;
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE))) return NULL;
    if(hfs_fsetattr(arg_file, arg_ent)) return NULL;
    return Py_None;
}

static PyObject *wrap_mkdir(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_mkdir(arg_vol, arg_path)) return NULL;
    return Py_None;
}

static PyObject *wrap_rmdir(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_rmdir(arg_vol, arg_path)) return NULL;
    return Py_None;
}

static PyObject *wrap_delete(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_delete(arg_vol, arg_path)) return NULL;
    return Py_None;
}

static PyObject *wrap_rename(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_srcpath; char *arg_dstpath;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_srcpath, &arg_dstpath)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_rename(arg_vol, arg_srcpath, arg_dstpath)) return NULL;
    return Py_None;
}

static PyObject *wrap_zero(PyObject *self, PyObject *args)
{
    char *arg_path; unsigned int arg_maxparts;
    unsigned long ret_blocks;
    if(!PyArg_ParseTuple(args, "sI", &arg_path, &arg_maxparts)) return NULL;
    if(hfs_zero(arg_path, arg_maxparts, &ret_blocks)) return NULL;
    return Py_BuildValue("k", ret_blocks);
}

static PyObject *wrap_mkpart(PyObject *self, PyObject *args)
{
    char *arg_path; unsigned long arg_len;
    if(!PyArg_ParseTuple(args, "sk", &arg_path, &arg_len)) return NULL;
    if(hfs_mkpart(arg_path, arg_len)) return NULL;
    return Py_None;
}

static PyObject *wrap_nparts(PyObject *self, PyObject *args)
{
    char *arg_path;
    int ret;
    if(!PyArg_ParseTuple(args, "s", &arg_path)) return NULL;
    ret = hfs_nparts(arg_path);
    if(ret == -1) return NULL;
    return Py_BuildValue("i", ret);
}

static PyObject *wrap_format(PyObject *self, PyObject *args) // bad blocks unimplemented
{
    char *arg_path; int arg_pnum; int arg_mode; char *arg_vname;
    if(!PyArg_ParseTuple(args, "siiy", &arg_path, &arg_pnum, &arg_mode, &arg_vname)) return NULL;
    if(hfs_format(arg_path, arg_pnum, arg_mode, arg_vname, 0, NULL)) return NULL;
    return Py_None;
}


static PyMethodDef module_methods[] = {
// Volume routines
    {"mount", wrap_mount, METH_VARARGS, ""},
    {"flush", wrap_flush, METH_VARARGS, ""},
    {"flushall", wrap_flushall, METH_NOARGS, ""},
    {"umount", wrap_umount, METH_VARARGS, ""},
    {"umountall", wrap_umountall, METH_NOARGS, ""},
    {"getvol", wrap_getvol, METH_VARARGS, ""},
    {"setvol", wrap_setvol, METH_VARARGS, ""},
    {"vstat", wrap_vstat, METH_VARARGS, ""},
    {"vsetattr", wrap_vsetattr, METH_VARARGS, ""},
// Directory routines
    {"chdir", wrap_chdir, METH_VARARGS, ""},
    {"getcwd", wrap_getcwd, METH_VARARGS, ""},
    {"setcwd", wrap_setcwd, METH_VARARGS, ""},
    {"dirinfo", wrap_dirinfo, METH_VARARGS, ""},
    {"opendir", wrap_opendir, METH_VARARGS, ""},
    {"readdir", wrap_readdir, METH_VARARGS, ""},
    {"closedir", wrap_closedir, METH_VARARGS, ""},
// File routines
    {"create", wrap_create, METH_VARARGS, ""},
    {"open", wrap_open, METH_VARARGS, ""},
    {"setfork", wrap_setfork, METH_VARARGS, ""},
    {"getfork", wrap_getfork, METH_VARARGS, ""},
    {"read", wrap_read, METH_VARARGS, ""},
    {"write", wrap_write, METH_VARARGS, ""},
    {"truncate", wrap_truncate, METH_VARARGS, ""},
    {"seek", wrap_seek, METH_VARARGS, ""},
    {"close", wrap_close, METH_VARARGS, ""},
// Catalog routines
    {"stat", wrap_stat, METH_VARARGS, ""},
    {"fstat", wrap_fstat, METH_VARARGS, ""},
    {"setattr", wrap_setattr, METH_VARARGS, ""},
    {"fsetattr", wrap_fsetattr, METH_VARARGS, ""},
    {"mkdir", wrap_mkdir, METH_VARARGS, ""},
    {"rmdir", wrap_rmdir, METH_VARARGS, ""},
    {"delete", wrap_delete, METH_VARARGS, ""},
    {"rename", wrap_rename, METH_VARARGS, ""},
// Media routines
    {"zero", wrap_zero, METH_VARARGS, ""},
    {"mkpart", wrap_mkpart, METH_VARARGS, ""},
    {"nparts", wrap_nparts, METH_VARARGS, ""},
    {"format", wrap_format, METH_VARARGS, ""},

    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef libhfs_module = {
    PyModuleDef_HEAD_INIT,
    "libhfs",
    "Test Module",
    -1,
    module_methods
};

PyMODINIT_FUNC PyInit_libhfs(void)
{
    return PyModule_Create(&libhfs_module);
}
