Read XML files and pull out selected values. Values to collect are
determined by paths found in a [structure file](#structure-file). The
structure file also includes a key which associates the values with a
parent element and names, which determine which file to place the
elements in.

Files can be passed as either gzipped or uncompressed XML files or from
standard in.

For more info on Pubmed's XML files see:
[pubmed<sub>190101</sub>.dtd.](https://dtd.nlm.nih.gov/ncbi/pubmed/doc/out/190101/index.html)

Usage:

``` bash
xml_read --cache-dir=cache --structure-file=structure.yml \
    data/*.xml.gz
```

Or, with python:

``` python
import pubmedparser
import pubmedparser.ftp

# Download data
files = pubmedparser.ftp.download(range(1, 6))

# Read XML files using a YAML file to describe what data to collect.
data_dir = "file_example"
structure_file = "example/structure.yml"
results = pubmedparser.read_xml(files, structure_file, data_dir)
```

See [the example
file](https://gitlab.com/net-synergy/pubmedparser/-/blob/master/example/creating_graphs.py)
for more options.

In python, the structure file can be replaced with a dictionary of
dictionaries as well.

## Building CLI

Requires `zlib`.

Clone the repository and in the directory run:

``` bash
make cli
```

or using nix:

``` bash
nix shell "gitlab:DavidRConnell/pubmedparser"
```

## Installing with pip

``` bash
pip install pubmedparser2
```

# Structure file

The structure file is a YAML file containing key-value pairs for
different tags and paths. There are two required keys: `root` and `key`.
`Root` provide the top-level tag, in the case of the pubmed files this
will be `PubmedArticleSet`.

``` bash
root: "/PubmedArticleSet"
```

The `/` is not strictly required as the program will ignore them, but
they are used to conform to the
[xpath](https://en.wikipedia.org/wiki/XPath) syntax (although this
program does not handle all cases for `xpath`).

Only tags below the root tag will be considered and the parsing will
terminate once the program has left the root of the tree.

`Key` is a reference tag. In the pubmed case, all data is with respect
to a publication, so the key should identify the publication the values
are linked to. The `PMID` tag is a suitable candidate.

``` bash
key: "/PubmedArticle/MedlineCitation/PMID"
```

After `root`, all paths are taken as relative to the root node.

The other name-pairs in the file determine what other items to collect.
These can either be a simple name and path, like the key, such as:

``` bash
Language: "/PubmedArticle/MedlineCitation/Article/Language"
Keywords: "/PubmedArticle/MedlineCitation/KeywordList/Keyword"
```

Or they can use a hierarchical representation to get multiple values
below a child. This is mainly used to handle lists of items where there
is an indefinite number of items below the list.

``` bash
Author: {
  root: "/PubmedArticle/MedlineCitation/Article/AuthorList",
  key: "/Author/auto_index",
  LastName: "/Author/LastName",
  ForeName: "/Author/ForeName",
  Affiliation: "/Author/AffiliationInfo/Affiliation",
  Orcid: "/Author/Identifier/[@Source='ORCID']"
}
```

Here, all paths are relative to the sub-structures `root` path, which is
in turn relative to the parent structure's `root`. This sub-structure
uses the same rules as the parent structure, so it needs both a `root`
and `key` name-value pair. The results of searching each path are
written to separate files. Each file gets a column for the parent and
child key. So in this case, each element of the author is linked by an
author key and that is related to the publication they authored through
the parent key.

The main parser is called recursively to parse this structure so it's
worth thinking about what the root should be under the context that the
parser will be called with that root. This means if, instead of stopping
at `/AuthorList`, `/Author` was added to the end of the root, the parser
would be called for each individual author, instead of once per author
list, leading to all author's getting the index 0.

There are a number of additional syntax constructs to note in the above
example. The key uses the special name `auto_index`, since there is no
author ID in the XML data, an index is used to count the authors in the
order they appear. This resets for each publication and starts at 0.
Treating the `auto_index` as the tail of a path allows you to control
when the indexing occurs—the index is incremented whenever it hits a
`/Author` tag.

In addition to the `auto_index` key, there is a second special index
name, `condensed`.

``` bash
Reference: {
  root: "/PubmedArticle/PubmedData/ReferenceList/Reference/ArticleIdList"
  key: "/condensed"
  PMID: "/ArticleId/[@IdType='pubmed']"
  DOI: "/ArticleId/[@IdType='doi']"
}
```

In the case of `condensed`, instead of writing the results to separate
files, they will printed as columns in the same file, and therefore do
not need an additional key for the sub-structure. If any of the elements
are missing, they will be left blank, for example, if the parser does
not find a pubmed ID for a given reference, the row will look like
`"%s\t\t%s"` where the first string will contain the parent key (the
`PMID` of the publication citing this reference) and the second string
will contain the reference's `DOI`.

The `/[@attribute='value']` syntax at the end of a path tells the parser
to only collect an element if it has an attribute and the attribute's
value matches the supplied value. Similarly the `/@attribute` syntax,
tells the parser to collect the value of the attribute `attribute` along
with the element's value. Then both values will be written to the output
file. Currently only a single attribute can be specified.

Lastly, there is a special syntax for writing condensed sub-structures:

``` bash
Date: "/PubmedArticle/MedlineCitation/Article/Journal/JournalIssue/PubDate/{Year,Month,Day}"
```

The `{child,child,child}` syntax allows you to select multiple children
at the same level to be printed to a single file. This is useful when
multiple children make up a single piece of information (i.e. the
publication date).

A similar example structure file can be found in the example directory
of this project at:
[file:./example/structure.yml](./example/structure.yml).

# Structure dictionary

The structure of the xml data to read can also be described as a python
dictionary of dictionaries.

The form is similar to the file:

``` python
structure = {
    "root": "//PubmedArticleSet",
    "key": "/PubmedArticle/MedlineCitation/PMID",
    "DOI": "/PubmedArticle/PubmedData/ArticleIdList/ArticleId/[@IdType='doi']",
    "Date": "/PubmedArticle/MedlineCitation/Article/Journal/JournalIssue/PubDate/{Year,Month,Day}",
    "Journal": "/PubmedArticle/MedlineCitation/Article/Journal/{Title,ISOAbbreviation}",
    "Language": "/PubmedArticle/MedlineCitation/Article/Language",
    "Author": {
        "root": "/PubmedArticle/MedlineCitation/Article/AuthorList",
        "key": "/Author/auto_index",
        "LastName": "/Author/LastName",
        "ForName": "/Author/ForeName",
        "Affiliation": "/Author/AffiliationInfo/Affiliation",
        "Orcid": "/Author/Identifier/[@Source='ORCID']",
    },
    "Grant": {
        "root": "/PubmedArticle/MedlineCitation/Article/GrantList",
        "key": "/Grant/auto_index",
        "ID": "/Grant/GrantID",
        "Agency": "/Grant/Agency",
    },
    "Chemical": "/PubmedArticle/MedlineCitation/ChemicalList/Chemical/NameOfSubstance/@UI",
    "Qualifier": "/PubmedArticle/MedlineCitation/MeshHeadingList/MeshHeading/QualifierName/@UI",
    "Descriptor": "/PubmedArticle/MedlineCitation/MeshHeadingList/MeshHeading/DescriptorName/@UI",
    "Keywords": "/PubmedArticle/MedlineCitation/KeywordList/Keyword",
    "Reference": {
        "root": (
            "/PubmedArticle/PubmedData/ReferenceList/Reference/ArticleIdList"
        ),
        "key": "/condensed",
        "PMID": "/ArticleId/[@IdType='pubmed']",
        "DOI": "/ArticleId/[@IdType='doi']",
    },
}
```

This can then be passed to `pubmedparser.read_xml` in place of the
structure file.

# Future goals

## Improve printing logic

Currently, values are printed as they are read in. Since the results for
the different paths are written to separate files, this shouldn't
matter, except for the case of the key. The key is not printed to its
own results file, instead whatever the last seen key was is printed as
the key for the current value being printed. If the key is not the first
element to be read in the subtree, there will be a mismatch between
value and publication ID.

In the case of `PMID` this is consistently the first element, so there
should not be a problem, however, it could be in other scenarios.

## Error handling

After refactoring the code, I have started adding some error handling
code, however this has not been consistently applied. Ideally, the
default behavior will be for functions to return error codes. Then use
an error checking macro to test that the result was not an error. I
would also like to add a set error strings that would be printed
depending on the error code. Possibly use a structure to represent
errors so that the erroring function could supply an additional string
along with the error.

Better error handling like this could also allow the python package to
write it's own error handling function in the C API to override the
default error mechanism to use python level errors. This would be done
by testing if an error handler function was defined, if so the error
checking macro would use that function, otherwise it would fallback to a
default function.
