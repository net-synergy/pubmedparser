import os

import pubmedparser
from pubmedparser.storage import default_cache_dir

structure_file = "example/structure.yml"
data_dir = "data/"
files = [
    os.path.join(data_dir, f)
    for f in os.listdir(data_dir)
    if f.endswith("xml")
]
cache_dir = "file_example"
abs_cache_dir = default_cache_dir(cache_dir)

# Read XML files using a YAML file to describe what data to collect.
pubmedparser.read_xml(
    files,
    structure_file,
    cache_dir,
)

os.listdir(abs_cache_dir)

structure_dict = {
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

cache_dir = "dict_example"
pubmedparser.read_xml(data_dir, structure_dict, cache_dir, exts=("xml.gz",))

abs_cache_dir = default_cache_dir(cache_dir)
os.listdir(abs_cache_dir)
