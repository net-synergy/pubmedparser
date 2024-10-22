#define PY_SSIZE_T_CLEAN

#include "read_xml.h"
#include "structure.h"

#include <Python.h>
#include <stdio.h>

#define STRMAX 10000

static char** files;
static path_struct ps;
size_t n_files;

static void file_list_destroy(char*** files, size_t n_files)
{
  for (size_t i = 0; i < n_files; i++) {
    free((*files)[i]);
  }
  free(*files);
}

static void py_warning_handler(char const* errstr, char const* msg)
{
  char buff[4096];
  snprintf(buff, sizeof(buff), "%s\n  %s\n", msg, errstr);
  PyErr_WarnEx(PyExc_RuntimeWarning, buff, 1);
}

static void py_error_handler(char const* errstr, char const* msg)
{
  PyObject* exc =
    pubmedparser_get_oom() ? PyExc_MemoryError : PyExc_RuntimeError;
  char buff[4096];
  snprintf(buff, sizeof(buff), "%s\n  %s\n", msg, errstr);

  file_list_destroy(&files, n_files);
  path_struct_destroy(ps);

  PyErr_SetString(exc, buff);
}

static void parse_file_list(PyObject* py_files, char*** files, size_t* n_files)
{
  if (!PyList_Check(py_files)) {
    PyErr_SetString(PyExc_ValueError, "Files argument was not a list.");
    return;
  }

  *n_files = (size_t)PyList_Size(py_files);
  *files = malloc(sizeof(**files) * *n_files);
  PyObject* f;
  for (size_t i = 0; i < *n_files; i++) {
    f = PyList_GetItem(py_files, (Py_ssize_t)i);
    if (!PyUnicode_Check(f)) {
      PyErr_SetString(PyExc_ValueError, "Files was not a list of strings.");
    }
    (*files)[i] = strdup(PyUnicode_AsUTF8(f));
  }
}

static PyObject* read_xml_from_structure_file(PyObject* _, PyObject* args)
{
  PyObject* files_obj;
  char const* structure_file;
  char const* cache_dir = "";
  char const* progress_file = "";
  int const n_threads = 0;
  int const overwrite_cache = 0;

  if (!PyArg_ParseTuple(args, "Osssip", &files_obj, &structure_file,
        &cache_dir, &progress_file, &n_threads, &overwrite_cache)) {
    return NULL;
  }

  pubmedparser_set_error_handler(py_error_handler);
  pubmedparser_set_warn_handler(py_warning_handler);

  parse_file_list(files_obj, &files, &n_files);
  ps = parse_structure_file(structure_file, STRMAX);
  read_xml(
    files, n_files, ps, cache_dir, overwrite_cache, progress_file, n_threads);
  file_list_destroy(&files, n_files);
  path_struct_destroy(ps);

  Py_RETURN_NONE;
}

static void reorder_ps(char const* name, size_t const pos, path_struct ps)
{
  size_t idx = 0;
  if (strcmp(ps->children[pos]->name, name) == 0) {
    return;
  }

  while (
    (idx < ps->n_children) && (strcmp(ps->children[idx]->name, name) != 0)) {
    idx++;
  }

  if (idx == ps->n_children) {
    size_t str_max = 1000;
    char errmsg[str_max + 1];
    strncpy(errmsg, "Structure dictionary missing required ", str_max);
    strncat(errmsg, name, str_max);
    strncat(errmsg, " key.", str_max);
    PyErr_SetString(PyExc_ValueError, errmsg);
    return;
  }

  path_struct child = ps->children[pos];
  ps->children[pos] = ps->children[idx];
  ps->children[idx] = child;
}

static void read_dict_values_i(path_struct ps, PyObject* dict)
{
  ps->n_children = (size_t)PyDict_Size(dict);
  ps->children = malloc(sizeof(*ps->children) * ps->n_children);

  // According the docs, pos is not consecutive for a dictionary so it can't be
  // used as the index.
  Py_ssize_t pos = 0;
  size_t idx = 0;
  PyObject *key, *value;
  path_struct child;
  while (PyDict_Next(dict, &pos, &key, &value)) {
    child = malloc(sizeof((*ps->children)[idx]));
    child->name = strdup(PyUnicode_AsUTF8(key));
    child->parent = ps;
    if (PyDict_Check(value)) {
      child->path = NULL;
      read_dict_values_i(child, value);
    } else {
      child->path = strdup(PyUnicode_AsUTF8(value));
      child->children = NULL;
      child->n_children = 0;
    }
    ps->children[idx] = child;
    idx++;
  }

  reorder_ps("root", 0, ps);
  reorder_ps("key", 1, ps);
}

static path_struct parse_structure_dictionary(PyObject* structure_dict)
{
  path_struct ps = malloc(sizeof(*ps));
  ps->name = strdup("top");
  ps->parent = NULL;
  ps->path = NULL;

  if (!(PyDict_Check(structure_dict))) {
    PyErr_SetString(
      PyExc_ValueError, "Structure dictionary was not a dictionary.");
    return NULL;
  }

  read_dict_values_i(ps, structure_dict);

  return ps;
}

static PyObject* read_xml_from_structure_dictionary(
  PyObject* _, PyObject* args)
{
  PyObject* files_obj;
  PyObject* structure_dict;
  char const* cache_dir = "";
  char const* progress_file = "";
  int const n_threads = 0;
  int const overwrite_cache = 0;

  if (!PyArg_ParseTuple(args, "OOssip", &files_obj, &structure_dict,
        &cache_dir, &progress_file, &n_threads, &overwrite_cache)) {
    return NULL;
  }

  pubmedparser_set_error_handler(py_error_handler);

  parse_file_list(files_obj, &files, &n_files);
  ps = parse_structure_dictionary(structure_dict);
  read_xml(
    files, n_files, ps, cache_dir, overwrite_cache, progress_file, n_threads);
  file_list_destroy(&files, n_files);
  path_struct_destroy(ps);

  Py_RETURN_NONE;
}

static PyMethodDef ReadXmlMethods[] = {
  {
    "from_structure_file",
    read_xml_from_structure_file,
    METH_VARARGS,
    "Read the provided XML files using a structure YAML file.",
  },
  {
    "from_structure_dictionary",
    read_xml_from_structure_dictionary,
    METH_VARARGS,
    "Read the provided XML files using a dictionary of dictionaries.",
  },
  { NULL, NULL, 0, NULL }
};

static struct PyModuleDef readxmlmodule = {
  .m_base = PyModuleDef_HEAD_INIT,
  .m_name = "_readxml",
  .m_doc = "Functions for reading XML files.",
  .m_size = -1,
  .m_methods = ReadXmlMethods,
};

PyMODINIT_FUNC PyInit__readxml(void)
{
  return PyModule_Create(&readxmlmodule);
}
