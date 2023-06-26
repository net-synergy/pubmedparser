import os

import pubmedparser
import pubmedparser.ftp

# Download data
files = pubmedparser.ftp.download(range(1383, 1389))

# Read XML files using a YAML file to describe what data to collect.
data_dir = "file_example"
structure_file = "example/structure.yml"
results = pubmedparser.read_xml(files, structure_file, data_dir)

os.listdir(results)

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

data_dir = "dict_example"
results = pubmedparser.read_xml(files, structure_dict, data_dir)

os.listdir(results)
