# -*- coding: utf-8 -*-
from setuptools import setup

packages = ["pubmedparser"]

package_data = {"": ["*"]}

setup_kwargs = {
    "name": "pubmedparser",
    "version": "0.1.0",
    "description": "Download and parse pubmed publication data",
    "long_description": (
        "#+TITLE: Pubmed parser\n#+PROPERTY: header-args:sh :eval"
        " no\n#+PROPERTY: header-args:bash :eval no :session *readme* :results"
        " none\n\nRead XML files and pull out selected values.\nValues to"
        " collect are determined by paths found in a"
        " [[#structure-file][structure file]].\nThe structure file also"
        " includes a key which associates the values with a parent element and"
        " names, which determine which file to place the elements in.\n\nFiles"
        " can be passed as either gzipped or uncompressed XML files or from"
        " standard in.\n\nFor more info on Pubmed's XML files see:"
        " [[https://dtd.nlm.nih.gov/ncbi/pubmed/doc/out/190101/index.html][pubmed_190101.dtd.]]\n\nUsage:\n#+begin_src"
        " sh :eval no\n  xml_read --cache-dir=cache"
        " --structure-file=structure.yml \\\n     "
        " data/*.xml.gz\n#+end_src\n\nThe cache directory is where the results"
        " are stored.\n\n** Building\nRequires ~zlib~.\n\nClone the repository"
        " and in the directory run:\n#+begin_src sh :eval no\n  make"
        " all\n#+end_src\n\nor using nix:\n#+begin_src sh :eval no\n  nix"
        ' shell "gitlab:DavidRConnell/pubmedparser"\n#+end_src\n\n* Structure'
        " file\n:PROPERTIES:\n:CUSTOM_ID: structure-file\n:header_args: eval"
        " no\n:END:\n\nThe structure file is a YAML file containing key-value"
        " pairs for different tags and paths.\nThere are two required keys:"
        " ~root~ and ~key~.\n~Root~ provide the top-level tag, in the case of"
        " the pubmed files this will be ~PubmedArticleSet~.\n\n#+begin_src sh"
        " :tangle ./example/structure.yml\n  root:"
        ' "/PubmedArticleSet"\n#+end_src\n\nThe ~/~ is not strictly required'
        " as the program will ignore them, but they are used to conform to the"
        " [[https://en.wikipedia.org/wiki/XPath][xpath]] syntax (although this"
        " program does not handle all cases for ~xpath~).\n\nOnly tags below"
        " the root tag will be considered and the parsing will terminate once"
        " the program has left the root of the tree.\n\n~Key~ is a reference"
        " tag.\nIn the pubmed case, all data is with respect to a publication,"
        " so the key should identify the publication the values are linked"
        " to.\nThe ~PMID~ tag is a suitable candidate.\n\n#+begin_src sh"
        " :tangle ./example/structure.yml\n  key:"
        ' "/PubmedArticle/MedlineCitation/PMID"\n#+end_src\n\nAfter ~root~,'
        " all paths are taken as relative to the root node.\n\nThe other"
        " name-pairs in the file determine what other items to collect.\nThese"
        " can either be a simple name and path, like the key, such"
        " as:\n\n#+begin_src sh :tangle ./example/structure.yml\n  Language:"
        ' "/PubmedArticle/MedlineCitation/Article/Language"\n  Keywords:'
        ' "/PubmedArticle/MedlineCitation/KeywordList/Keyword"\n#+end_src\n\nOr'
        " they can use a hierarchical representation to get multiple values"
        " below a child.\nThis is mainly used to handle lists of items where"
        " there is an indefinite number of items below the"
        " list.\n\n#+begin_src sh :tangle ./example/structure.yml\n  Author:"
        ' {\n    root: "/PubmedArticle/MedlineCitation/Article/AuthorList",\n '
        '   key: "/Author/auto_index",\n    LastName: "/Author/LastName",\n   '
        ' ForeName: "/Author/ForeName",\n    Affiliation:'
        ' "/Author/AffiliationInfo/Affiliation",\n    Orcid:'
        " \"/Author/Identifier/[@Source='ORCID']\"\n  }\n#+end_src\n\nHere,"
        " all paths are relative to the sub-structures ~root~ path, which is"
        " in turn relative to the parent structure's ~root~.\nThis"
        " sub-structure uses the same rules as the parent structure, so it"
        " needs both a ~root~ and ~key~ name-value pair.\nThe results of"
        " searching each path are written to separate files.\nEach file gets a"
        " column for the parent and child key.\nSo in this case, each element"
        " of the author is linked by an author key and that is related to the"
        " publication they authored through the parent key.\n\nThe main parser"
        " is called recursively to parse this structure so it's worth"
        " thinking about what the root should be under the context that the"
        " parser will be called with that root.\nThis means if, instead of"
        " stopping at ~/AuthorList~, ~/Author~ was added to the end of the"
        " root, the parser would be called for each individual author, instead"
        " of once per author list, leading to all author's getting the index"
        " 0.\n\nThere are a number of additional syntax constructs to note in"
        " the above example.\nThe key uses the special name ~auto_index~,"
        " since there is no author ID in the XML data, an index is used to"
        " count the authors in the order they appear.\nThis resets for each"
        " publication and starts at 0.\nTreating the ~auto_index~ as the tail"
        " of a path allows you to control when the indexing occurs---the index"
        " is incremented whenever it hits a ~/Author~ tag.\n\nIn addition to"
        " the ~auto_index~ key, there is a second special index name,"
        " ~condensed~.\n\n#+begin_src sh :tangle ./example/structure.yml\n "
        " Reference: {\n    root:"
        ' "/PubmedArticle/PubmedData/ReferenceList/Reference/ArticleIdList"\n '
        '   key: "/condensed"\n    PMID: "/ArticleId/[@IdType=\'pubmed\']"\n  '
        "  DOI: \"/ArticleId/[@IdType='doi']\"\n  }\n#+end_src\n\nIn the case"
        " of ~condensed~, instead of writing the results to separate files,"
        " they will printed as columns in the same file, and therefore do not"
        " need an additional key for the sub-structure.\nIf any of the"
        " elements are missing, they will be left blank, for example, if the"
        " parser does not find a pubmed ID for a given reference, the row will"
        ' look like ~"%s\\t\\t%s"~ where the first string will contain the'
        " parent key (the ~PMID~ of the publication citing this reference) and"
        " the second string will contain the reference's ~DOI~.\n\nThe"
        " ~/[@attribute='value']~ syntax at the end of a path tells the"
        " parser to only collect an element if it has an attribute and the"
        " attribute's value matches the supplied value.\nSimilarly the"
        " ~/@attribute~ syntax, tells the parser to collect the value of the"
        " attribute ~attribute~ along with the element's value.\nThen both"
        " values will be written to the output file.\nCurrently only a single"
        " attribute can be specified.\n\nLastly, there is a special syntax for"
        " writing condensed sub-structures:\n\n#+begin_src sh :tangle"
        " ./example/structure.yml\n  Date:"
        ' "/PubmedArticle/MedlineCitation/Article/Journal/JournalIssue/PubDate/{Year,Month,Day}"\n#+end_src\n\nThe'
        " ~{child,child,child}~ syntax allows you to select multiple children"
        " at the same level to be printed to a single file.\nThis is useful"
        " when multiple children make up a single piece of information (i.e."
        " the publication date).\n\nA similar example structure file can be"
        " found in the example directory of this project at:"
        " [[file:./example/structure.yml]].\n* Future goals\n** Improve"
        " printing logic\nCurrently, values are printed as they are read in."
        " Since the results for the different paths are written to separate"
        " files, this shouldn't matter, except for the case of the key.\nThe"
        " key is not printed to its own results file, instead whatever the"
        " last seen key was is printed as the key for the current value being"
        " printed.\nIf the key is not the first element to be read in the"
        " subtree, there will be a mismatch between value and publication"
        " ID.\n\nIn the case of ~PMID~ this is consistently the first element,"
        " so there should not be a problem, however, it could be in other"
        " scenarios.\n** Error handling\nAfter refactoring the code, I have"
        " started adding some error handling code, however this has not been"
        " consistently applied.\nIdeally, the default behavior will be for"
        " functions to return error codes.\nThen use an error checking macro"
        " to test that the result was not an error.\nI would also like to add"
        " a set error strings that would be printed depending on the error"
        " code.\nPossibly use a structure to represent errors so that the"
        " erroring function could supply an additional string along with the"
        " error.\n\nBetter error handling like this could also allow the"
        " python package to write it's own error handling function in the C"
        " API to override the default error mechanism to use python level"
        " errors.\nThis would be done by testing if an error handler function"
        " was defined, if so the error checking macro would use that function,"
        " otherwise it would fallback to a default function.\n"
    ),
    "author": "David Connell",
    "author_email": "davidconnell12@gmail.com",
    "maintainer": "David Connell",
    "maintainer_email": "davidconnell12@gmail.com",
    "url": "https://gitlab.com/net-synergy/pubmedparser",
    "packages": packages,
    "package_data": package_data,
    "python_requires": ">=3.8,<4.0",
}
from build import *

build(setup_kwargs)

setup(**setup_kwargs)
