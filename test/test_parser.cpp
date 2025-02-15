// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
///////////////////////////////////////////////////////////////////////////////
//
// This file is a part of UPPAAL.
// Copyright (c) 2021-2022, Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////////////////

/**
 * File:   test_parser.cpp
 * Author: Rasmus R. Kjær
 *
 * Created on 20 August 2021, 09:47
 */

#include "utap/StatementBuilder.hpp"
#include "utap/utap.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

inline std::string read_content(const std::string& file_name)
{
    const auto path = std::filesystem::path{MODELS_DIR} / file_name;
    auto ifs = std::ifstream{path};
    auto content = std::string{std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{}};
    REQUIRE(!content.empty());
    return content;
}

std::unique_ptr<UTAP::Document> read_document(const std::string& file_name)
{
    auto doc = std::make_unique<UTAP::Document>();
    auto res = parseXMLBuffer(read_content(file_name).c_str(), doc.get(), true);
    REQUIRE(res == 0);
    return doc;
}

TEST_CASE("Double Serialization Test")
{
    auto doc = read_document("if_statement.xml");
    REQUIRE(doc);
    CHECK(doc->getErrors().size() == 0);
    CHECK(doc->getWarnings().size() == 0);
}

TEST_CASE("Power expressions")
{
    auto doc = read_document("powers.xml");
    REQUIRE(doc);
    CHECK(doc->getErrors().size() == 0);
    CHECK(doc->getWarnings().size() == 0);
}

TEST_CASE("External functions")
{
    auto doc = read_document("external_fn.xml");
    REQUIRE(doc);
    auto& errs = doc->getErrors();
    auto& warns = doc->getWarnings();
    REQUIRE(errs.size() == 3);  // "libbad" not found (x2), "absent" undefined.
    CHECK(warns.size() == 0);
}

class QueryBuilder : public UTAP::StatementBuilder
{
    UTAP::expression_t query;

public:
    explicit QueryBuilder(UTAP::Document& doc): UTAP::StatementBuilder{doc} {}
    void property() override
    {
        REQUIRE(fragments.size() > 0);
        query = fragments[0];
        fragments.pop();
    }
    [[nodiscard]] UTAP::expression_t getQuery() const { return query; }
    UTAP::variable_t* addVariable(UTAP::type_t type, const std::string& name, UTAP::expression_t init,
                                  UTAP::position_t pos) override
    {
        throw UTAP::NotSupportedException("addVariable is not supported");
    }
    bool addFunction(UTAP::type_t type, const std::string& name, UTAP::position_t pos) override
    {
        throw UTAP::NotSupportedException("addFunction is not supported");
    }
};

TEST_CASE("SMC bounds in queries")
{
    auto doc = std::make_unique<UTAP::Document>();
    auto builder = std::make_unique<QueryBuilder>(*doc);
    SUBCASE("Probability estimation query with 7 runs")
    {
        auto res = parseProperty("Pr[<=1;7](<> true)", builder.get());
        REQUIRE(res == 0);
        auto expr = builder->getQuery();
        REQUIRE(expr.getSize() == 5);
        CHECK(expr.get(0).getValue() == 7);  // number of runs
    }
    SUBCASE("Probability estimation query without runs")
    {
        auto res = parseProperty("Pr[<=1](<> true)", builder.get());
        REQUIRE(res == 0);
        auto expr = builder->getQuery();
        REQUIRE(expr.getSize() == 5);
        CHECK(expr.get(0).getValue() == -1);  // number of runs
    }
    SUBCASE("Value estimation query with 7 runs")
    {
        auto res = parseProperty("E[<=1;7](max: 1)", builder.get());
        REQUIRE(res == 0);
        auto expr = builder->getQuery();
        REQUIRE(expr.getSize() == 5);
        CHECK(expr.get(0).getValue() == 7);  // number of runs
    }
    SUBCASE("Value estimation query without runs")
    {
        auto res = parseProperty("E[<=1](max: 1)", builder.get());
        REQUIRE(res == 0);
        auto expr = builder->getQuery();
        REQUIRE(expr.getSize() == 5);
        CHECK(expr.get(0).getValue() == -1);  // number of runs
    }
}
