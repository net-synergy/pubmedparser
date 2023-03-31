#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <omp.h>

#include "read_xml.h"
#include "structure.h"

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
  size_t str_max = 10000;

  PyObject *files;
  const char *structure_file;
  const char *cache_dir;
  const char *progress_file;
  const int n_threads;
  char **files_i;
  size_t n_threads_i;
  size_t n_files_i;
  path_struct ps;
  int status;

  if (!PyArg_ParseTuple(args, "Osssi", &files, &structure_file, &cache_dir,
                        &progress_file, &n_threads)) {
    return NULL;
  }

  if (!PyList_Check(files)) {
    PyErr_SetString(PyExc_ValueError, "Files argument was not a list.");
  }

  n_files_i = (size_t)PyList_Size(files);
  files_i = malloc(sizeof(*files_i) * n_files_i);
  PyObject *f;
  for (size_t i = 0; i < n_files_i; i++) {
    f = PyList_GetItem(files, (Py_ssize_t)i);
    if (!PyUnicode_Check(f)) {
      PyErr_SetString(PyExc_ValueError, "Files was not a list of strings.");
    }
    files_i[i] = strdup(PyUnicode_AsUTF8(f));
  }

  n_threads_i = determine_n_threads(n_threads);
  ps = parse_structure_file(structure_file, str_max);
  status = read_xml(files_i, n_files_i, ps, cache_dir, progress_file,
                    n_threads_i);

  for (size_t i = 0; i < n_files_i; i++) {
    free(files_i[i]);
  }
  free(files_i);

  // read_xml exits on error so should never enter this if statement until the
  // pubmedparser C lib gets proper error handling.
  if (status > 0) {
    PyErr_SetString(PyExc_EOFError,
                    "One or more XML files was not formatted correctly");
    return NULL;
  }

  Py_RETURN_NONE;
}

static path_struct parse_structure_dictionary(PyObject *args)
{
  path_struct ps = malloc(sizeof(*ps));
  return ps;
}

static PyObject *read_xml_from_structure_dictionary(PyObject *self,
    PyObject *args)
{

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
