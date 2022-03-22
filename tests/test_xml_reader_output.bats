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

@test "Test key nodes file" {
    # NOTE: Don't know why `echo` is passing a literal '\n' instead of a
    # tab character. Using `tr` to get around.

    diff $cache_dir/Publication.tsv <(echo "1 2" | tr ' ' '\n')
}

@test "Test tag mismatch causes error" {
    run read_xml \
	--structure-file=$structure_file \
	--cache-dir=$cache_dir <<EOF
<PubmedArticleSet>
 <PubmedArticle>
</PubmedArticleSet>
EOF

    [ "$status" -eq 1 ]
    [ "$output" = "Open and closing tags did not match." ]
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

@test "Test handle missing fore name" {
    diff <(cut -f1 -c $cache_dir/Author.tsv | sort) \
	<(cat "Smith\tJohn\nDoe\tJane\n\tJake\nSmith\tJohn\n" | \
	sort)
}

@test "Test collects attributes" {
    diff $cache_dir/Chemical.tsv \
	<(echo "1 D1 Molecule_1 D2 Solution" | tr ' ' '\t' | tr '_' '\n')
}

@test "Test filtering by attribute value" {
    # If it collects references with doi attribute as well as pubmed,
    # there will be more entries i.e. 10.000 and 10.001.
    diff $cache_dir/Reference.tsv  <(echo "1 2" | tr ' ' '\t')
}
