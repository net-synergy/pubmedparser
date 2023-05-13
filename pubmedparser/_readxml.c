#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <omp.h>

#include "read_xml.h"
#include "structure.h"

#define STRMAX 10000

static void parse_file_list(PyObject *py_files, char ***files,
                            size_t *n_files)
{
  if (!PyList_Check(py_files)) {
    PyErr_SetString(PyExc_ValueError, "Files argument was not a list.");
    return NULL;
  }

  *n_files = (size_t)PyList_Size(py_files);
  *files = malloc(sizeof(**files) * *n_files);
  PyObject *f;
  for (size_t i = 0; i < *n_files; i++) {
    f = PyList_GetItem(py_files, (Py_ssize_t)i);
    if (!PyUnicode_Check(f)) {
      PyErr_SetString(PyExc_ValueError, "Files was not a list of strings.");
    }
    (*files)[i] = strdup(PyUnicode_AsUTF8(f));
  }
}

static void destroy_file_list(char ***files, size_t n_files)
{
  for (size_t i = 0; i < n_files; i++) {
    free((*files)[i]);
  }
  free(*files);
}

static size_t determine_n_threads(int n_threads)
{
  size_t n_threads_i = (size_t)n_threads;
  if (n_threads == -1) {
    #pragma omp parallel
    {
      #pragma omp single
      n_threads_i = (size_t)omp_get_num_threads();
    }
  }
  return n_threads_i;
}

static PyObject *read_xml_from_structure_file(PyObject *self, PyObject *args)
{
  PyObject *files;
  const char *structure_file;
  const char *cache_dir;
  const char *progress_file;
  const int n_threads;
  const int overwrite_cache;
  char **files_i;
  size_t n_threads_i;
  size_t n_files_i;
  path_struct ps;
  int status;

  if (!PyArg_ParseTuple(args, "Osssip", &files, &structure_file, &cache_dir,
                        &progress_file, &n_threads, &overwrite_cache)) {
    return NULL;
  }

  parse_file_list(files, &files_i, &n_files_i);
  n_threads_i = determine_n_threads(n_threads);
  ps = parse_structure_file(structure_file, STRMAX);
  status = read_xml(files_i, n_files_i, ps, cache_dir, overwrite_cache,
                    progress_file, n_threads_i);
  destroy_file_list(&files_i, n_files_i);
  path_struct_destroy(ps);

  // read_xml exits on error so should never enter this if statement until the
  // pubmedparser C lib gets proper error handling.
  if (status > 0) {
    PyErr_SetString(PyExc_EOFError,
                    "One or more XML files was not formatted correctly");
    return NULL;
  }

  Py_RETURN_NONE;
}

static void reorder_ps(const char *name, const size_t pos, path_struct ps)
{
  size_t idx = 0;
  if (strcmp(ps->children[pos]->name, name) == 0) {
    return;
  }

  while ((idx < ps->n_children) &&
         (strcmp(ps->children[idx]->name, name) != 0)) {
    idx++;
  }

  if (idx == ps->n_children) {
    size_t str_max = 1000;
    char errmsg[str_max + 1];
    strncpy(errmsg, "Structure dictionary missing required ", str_max);
    strncat(errmsg, name, str_max);
    strncat(errmsg, " key.", str_max);
    PyErr_SetString(PyExc_ValueError, errmsg);
    return NULL;
  }

  path_struct child = ps->children[pos];
  ps->children[pos] = ps->children[idx];
  ps->children[idx] = child;
}

static void read_dict_values_i(path_struct ps, PyObject *dict)
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

static path_struct parse_structure_dictionary(PyObject *structure_dict)
{
  path_struct ps = malloc(sizeof(*ps));
  ps->name = strdup("top");
  ps->parent = NULL;
  ps->path = NULL;

  if (!(PyDict_Check(structure_dict))) {
    PyErr_SetString(PyExc_ValueError,
                    "Structure dictionary was not a dictionary.");
    return NULL;
  }

  read_dict_values_i(ps, structure_dict);

  return ps;
}

static PyObject *read_xml_from_structure_dictionary(PyObject *self,
    PyObject *args)
{
  PyObject *files;
  PyObject *structure_dict;
  const char *cache_dir;
  const char *progress_file;
  const int n_threads;
  const int overwrite_cache;
  char **files_i;
  size_t n_threads_i;
  size_t n_files_i;
  path_struct ps;
  int status;

  if (!PyArg_ParseTuple(args, "OOssip", &files, &structure_dict, &cache_dir,
                        &progress_file, &n_threads, &overwrite_cache)) {
    return NULL;
  }

  parse_file_list(files, &files_i, &n_files_i);
  n_threads_i = determine_n_threads(n_threads);
  ps = parse_structure_dictionary(structure_dict);
  status = read_xml(files_i, n_files_i, ps, cache_dir, overwrite_cache,
                    progress_file, n_threads_i);
  destroy_file_list(&files_i, n_files_i);
  path_struct_destroy(ps);

  // read_xml exits on error so should never enter this if statement until the
  // pubmedparser C lib gets proper error handling.
  if (status > 0) {
    PyErr_SetString(PyExc_EOFError,
                    "One or more XML files was not formatted correctly");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyMethodDef ReadXmlMethods[] = {
  {
    "from_structure_file", read_xml_from_structure_file, METH_VARARGS,
    "Read the provided XML files using a structure YAML file."
  },
  {
    "from_structure_dictionary", read_xml_from_structure_dictionary, METH_VARARGS,
    "Read the provided XML files using a dictionary of dictionaries."
  },
  {NULL, NULL, 0, NULL}
};

static struct PyModuleDef readxmlmodule = {
  PyModuleDef_HEAD_INIT,
  "_readxml",
  "Functions for reading XML files.",
  -1,
  ReadXmlMethods
};

PyMODINIT_FUNC PyInit__readxml(void)
{
  return PyModule_Create(&readxmlmodule);
}
