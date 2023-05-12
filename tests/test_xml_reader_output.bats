#! /usr/bin/env bats

setup() {
    load 'bats_helpers'
    _common_setup
}

setup_file() {
    load 'bats_helpers'
    _common_file_setup

    read_xml --structure-file=$structure_file --cache-dir=$cache_dir <<EOF
<PubmedArticleSet>
  <PubmedArticle>
    <MedlineCitation Status="PubMed-not-MEDLINE" Owner="NLM">
      <PMID Version="1">1</PMID>
      <Article PubModel="Electronic-eCollection">
	<Journal>
	  <JournalIssue CitedMedium="Print">
	    <PubDate>
	      <Year>2022</Year>
	    </PubDate>
	  </JournalIssue>
	</Journal>
	<Abstract>
	  <AbstractText>123456789<sub>10</sub>111213141516171819202122232425262728293031323334353637383940414243444546474849505152535455565758596061626364656667686970</AbstractText>
	</Abstract>
	<AuthorList CompleteYN="Y">
	  <Author ValidYN="Y">
	    <LastName>John</LastName>
	    <ForeName>Smith</ForeName>
	  </Author>
	  <Author ValidYN="Y">
	    <LastName>Jane</LastName>
	    <ForeName>Doe</ForeName>
	    <Initials>A</Initials>
	  </Author>
	  <Author ValidYN="Y">
	    <LastName>Jake</LastName>
	  </Author>
	</AuthorList>
	<Language>eng</Language>
      </Article>
      <ChemicalList>
        <Chemical>
	  <NameOfSubstance @UI="D1">Molecule</NameOfSubstance>
	  <NameOfSubstance @UI="D2">Solution</NameOfSubstance>
        </Chemical>
      </ChemicalList>
      <KeywordList>
	  <Keyword> Spacey  </Keyword>
	  <Keyword>Lines
are a
pain
</Keyword>
      </KeywordList>
    </MedlineCitation>
    <PubmedData>
      <ReferenceList>
        <Reference>
          <ArticleIdList>
	    <ArticleId IdType="pubmed">2</ArticleId>
	    <ArticleId IdType="doi">10.000</ArticleId>
          </ArticleIdList>
        </Reference>
      </ReferenceList>
    </PubmedData>
  </PubmedArticle>
  <PubmedArticle>
    <MedlineCitation Status="PubMed-not-MEDLINE" Owner="NLM">
      <PMID Version="1">2</PMID>
      <Article PubModel="Electronic-eCollection">
	<Journal>
	  <JournalIssue CitedMedium="Print">
	    <PubDate>
	      <Year>2022</Year>
	    </PubDate>
	  </JournalIssue>
	</Journal>
	<AuthorList CompleteYN="Y">
	  <Author ValidYN="Y">
	    <LastName>John</LastName>
	    <ForeName>Smith</ForeName>
	  </Author>
	</AuthorList>
      </Article>
    </MedlineCitation>
    <PubmedData>
      <ReferenceList>
        <Reference>
          <ArticleIdList>
	    <ArticleId IdType="doi">10.0001</ArticleId>
          </ArticleIdList>
        </Reference>
      </ReferenceList>
    </PubmedData>
  </PubmedArticle>
</PubmedArticleSet>
EOF
}

teardown_file() {
    load 'bats_helpers'
    _common_file_teardown
}

@test "Test tag mismatch causes error" {
    run read_xml \
        --structure-file=$structure_file \
        --cache-dir=$cache_dir <<EOF
<PubmedArticleSet>
 <PubmedArticle>
</PubmedArticleSet>
EOF

    [ "$status" -eq 4 ] # 4 is PP_ERR_TAG_MISMATCH
    [ "$output" = "Tags in XML file did not match

Error in file -" ]
}

@test "Test handle empty tag" {
    # Would return a tag mismatch error if `<blah />` is not skipped.
    read_xml \
        --structure-file=$structure_file \
        --cache-dir=$cache_dir <<EOF
<PubmedArticleSet>
 <PubmedArticle>
  <blah />
 </PubmedArticle>
</PubmedArticleSet>
EOF
}

@test "Test handle empty tag in value field" {
    # Would return a tag mismatch error if `<blah />` is not skipped.
    read_xml \
        --structure-file=$structure_file \
        --cache-dir=$cache_dir <<EOF
<PubmedArticleSet>
 <PubmedArticle>
    <MedlineCitation Status="PubMed-not-MEDLINE" Owner="NLM">
      <PMID Version="1">1</PMID>
      <Article PubModel="Electronic-eCollection">
	<Abstract>
	  <AbstractText>123456789<sub>10</sub>1112131<blah/>70</AbstractText>
	</Abstract>
      </Article>
    </MedlineCitation>
 </PubmedArticle>
</PubmedArticleSet>
EOF
}

@test "Test creates regular headers" {
    diff <(head -n1 $cache_dir/Language.tsv) <(echo -e "PMID\tLanguage")
    diff <(head -n1 $cache_dir/Keywords.tsv) <(echo -e "PMID\tKeywords")
}

@test "Test creates condensed headers" {
    diff <(head -n1 $cache_dir/Date.tsv) <(echo -e "PMID\tYear\tMonth\tDay")
}

@test "Test creates header with attribute" {
    diff <(head -n1 $cache_dir/Chemical.tsv) <(echo -e "PMID\tChemical\tUI")
}

@test "Test creates header for recursive node" {
    diff <(head -n1 $cache_dir/Author_LastName.tsv) <(echo -e "PMID\tAuthorID\tLastName")
    diff <(head -n1 $cache_dir/Author_ForeName.tsv) <(echo -e "PMID\tAuthorID\tForeName")
}

@test "Test collects attributes" {
    diff $cache_dir/Chemical.tsv \
        <(echo -e "PMID\tChemical\tUI\n1\tMolecule\tD1\n1\tSolution\tD2")
}

@test "Test filtering by attribute value" {
    # If it collects references with doi attribute as well as pubmed,
    # there will be more entries i.e. 10.000 and 10.001.
    diff <(sed -n '2,$p' <$cache_dir/Reference.tsv) <(echo -e "1\t2")
}

@test "Test reads values with HTML tags in them" {
    grep -oe '9<sub>10</sub>' $cache_dir/Abstract.tsv
}

@test "Test trims spaces in value field" {
    diff <(sed -n '2p' <$cache_dir/Keywords.tsv) <(echo -e "1\tSpacey")
}

@test "Test handles newlines in value field and replaces with spaces" {
    diff <(tail -n1 $cache_dir/Keywords.tsv) <(echo -e "1\tLines are a pain")
}

# @test "Test reads values greater than VALUE_MAX" {
#     # NOTE: VALUE_MAX turned down to 100 for check.
#     # FIXME: VALUE_MAX currently not implemented so this does not test anything.
#     diff $cache_dir/Abstract.tsv <(echo -e "1\t123456789<sub>10</sub>111213141516171819202122232425262728293031323334353637383940414243444546474849505152535455565758596061626364656667686970")
# }
