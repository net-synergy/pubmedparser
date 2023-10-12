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
    "Publication": {
        "root": "/PubmedArticle/MedlineCitation/Article/Journal",
        "key": "/condensed",
        "Year": "/JournalIssue/PubDate/Year",
        "Month": "/JournalIssue/PubDate/Month",
        "Day": "/JournalIssue/PubDate/Day",
        "Journal": "/Title",
    },
    "Language": "/PubmedArticle/MedlineCitation/Article/Language",
    "Author": {
        "root": "/PubmedArticle/MedlineCitation/Article/AuthorList",
        "key": "/Author/auto_index",
        "LastName": "/Author/LastName",
        "ForeName": "/Author/ForeName",
        "Affiliation": "/Author/AffiliationInfo/Affiliation",
        "Orcid": "/Author/Identifier/[@Source='ORCID']",
    },
    "Grant": "/PubmedArticle/MedlineCitation/Article/GrantList/Grant/{GrantID,Agency}",
    "Chemical": "/PubmedArticle/MedlineCitation/ChemicalList/Chemical/NameOfSubstance/@UI",
    "Qualifier": "/PubmedArticle/MedlineCitation/MeshHeadingList/MeshHeading/QualifierName/@UI",
    "Descriptor": "/PubmedArticle/MedlineCitation/MeshHeadingList/MeshHeading/DescriptorName/@UI",
    "Keywords": "/PubmedArticle/MedlineCitation/KeywordList/Keyword",
    "Reference": "/PubmedArticle/PubmedData/ReferenceList/Reference/ArticleIdList/ArticleId/[@IdType='pubmed']",
}

# Read XML files using a dictionary to describe what data to collect.
data_dir = "dict_example"
results = pubmedparser.read_xml(files, structure_dict, data_dir)

os.listdir(results)
