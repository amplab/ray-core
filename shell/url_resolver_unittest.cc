// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/url_resolver.h"

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace shell {
namespace test {
namespace {

typedef testing::Test URLResolverTest;

TEST_F(URLResolverTest, MojoURLsFallThrough) {
  URLResolver resolver;
  resolver.AddURLMapping(GURL("mojo:test"), GURL("mojo:foo"));
  const GURL base_url("file:/base");
  resolver.SetMojoBaseURL(base_url);
  GURL mapped_url = resolver.ApplyMappings(GURL("mojo:test"));
  std::string resolved(resolver.ResolveMojoURL(mapped_url).spec());
  // Resolved must start with |base_url|.
  EXPECT_EQ(base_url.spec(), resolved.substr(0, base_url.spec().size()));
  // And must contain foo.
  EXPECT_NE(std::string::npos, resolved.find("foo"));
}

TEST_F(URLResolverTest, MapURL) {
  URLResolver resolver;
  resolver.AddURLMapping(GURL("https://domokit.org/test.mojo"),
                         GURL("file:///mojo/src/out/Debug/test.mojo"));
  GURL mapped_url =
      resolver.ApplyMappings(GURL("https://domokit.org/test.mojo"));
  EXPECT_EQ("file:///mojo/src/out/Debug/test.mojo", mapped_url.spec());
}

TEST_F(URLResolverTest, MultipleMapURL) {
  URLResolver resolver;
  resolver.AddURLMapping(GURL("https://a.org/foo"),
                         GURL("https://b.org/a/foo"));
  resolver.AddURLMapping(GURL("https://b.org/a/foo"),
                         GURL("https://c.org/b/a/foo"));
  GURL mapped_url = resolver.ApplyMappings(GURL("https://a.org/foo"));
  EXPECT_EQ("https://c.org/b/a/foo", mapped_url.spec());
}

TEST_F(URLResolverTest, MapOrigin) {
  URLResolver resolver;
  resolver.AddOriginMapping(GURL("https://domokit.org"),
                            GURL("file:///mojo/src/out/Debug"));
  GURL mapped_url =
      resolver.ApplyMappings(GURL("https://domokit.org/test.mojo"));
  EXPECT_EQ("file:///mojo/src/out/Debug/test.mojo", mapped_url.spec());
}

TEST_F(URLResolverTest, MultipleMapOrigin) {
  URLResolver resolver;
  resolver.AddOriginMapping(GURL("https://a.org"), GURL("https://b.org/a"));
  resolver.AddOriginMapping(GURL("https://b.org"), GURL("https://c.org/b"));
  GURL mapped_url = resolver.ApplyMappings(GURL("https://a.org/foo"));
  EXPECT_EQ("https://c.org/b/a/foo", mapped_url.spec());
}

TEST_F(URLResolverTest, MapOriginThenURL) {
  URLResolver resolver;
  resolver.AddOriginMapping(GURL("https://a.org"), GURL("https://b.org/a"));
  resolver.AddURLMapping(GURL("https://b.org/a/foo"),
                         GURL("https://c.org/b/a/foo"));
  GURL mapped_url = resolver.ApplyMappings(GURL("https://a.org/foo"));
  EXPECT_EQ("https://c.org/b/a/foo", mapped_url.spec());
}

TEST_F(URLResolverTest, MapURLThenOrigin) {
  URLResolver resolver;
  resolver.AddURLMapping(GURL("https://a.org/foo"),
                         GURL("https://b.org/a/foo"));
  resolver.AddOriginMapping(GURL("https://b.org"), GURL("https://c.org/b"));
  GURL mapped_url = resolver.ApplyMappings(GURL("https://a.org/foo"));
  EXPECT_EQ("https://c.org/b/a/foo", mapped_url.spec());
}

TEST_F(URLResolverTest, GetOriginMappings) {
  std::vector<std::string> args;
  args.push_back("--map-origin=https://a.org=https://b.org/a");
  std::vector<URLResolver::OriginMapping> mappings =
      URLResolver::GetOriginMappings(args);
  ASSERT_EQ(1U, mappings.size());
  EXPECT_EQ("https://a.org", mappings[0].origin);
  EXPECT_EQ("https://b.org/a", mappings[0].base_url);

  args.clear();
  args.push_back("-map-origin=https://a.org=https://b.org/a");
  mappings = URLResolver::GetOriginMappings(args);
  ASSERT_EQ(1U, mappings.size());
  EXPECT_EQ("https://a.org", mappings[0].origin);
  EXPECT_EQ("https://b.org/a", mappings[0].base_url);

  args.clear();
  args.push_back("--map-origin");
  mappings = URLResolver::GetOriginMappings(args);
  EXPECT_EQ(0U, mappings.size());

  args.clear();
  args.push_back("--map-origin=");
  mappings = URLResolver::GetOriginMappings(args);
  EXPECT_EQ(0U, mappings.size());

  args.clear();
  args.push_back("mojo_shell");
  args.push_back("--args-for=https://a.org/foo --test");
  args.push_back("--map-origin=https://a.org=https://b.org/a");
  args.push_back("--map-origin=https://b.org=https://c.org/b");
  args.push_back("https://a.org/foo");
  mappings = URLResolver::GetOriginMappings(args);
  ASSERT_EQ(2U, mappings.size());
  EXPECT_EQ("https://a.org", mappings[0].origin);
  EXPECT_EQ("https://b.org/a", mappings[0].base_url);
  EXPECT_EQ("https://b.org", mappings[1].origin);
  EXPECT_EQ("https://c.org/b", mappings[1].base_url);
}

TEST_F(URLResolverTest, TestQueryForURLMapping) {
  URLResolver resolver;
  resolver.SetMojoBaseURL(GURL("file:/base"));
  resolver.AddURLMapping(GURL("https://a.org/foo"),
                         GURL("https://b.org/a/foo"));
  resolver.AddURLMapping(GURL("https://b.org/a/foo"),
                         GURL("https://c.org/b/a/foo"));
  GURL mapped_url = resolver.ApplyMappings(GURL("https://a.org/foo?a=b"));
  EXPECT_EQ("https://c.org/b/a/foo?a=b", mapped_url.spec());
}

TEST_F(URLResolverTest, TestQueryForBaseURL) {
  URLResolver resolver;
  resolver.SetMojoBaseURL(GURL("file:///base"));
  GURL mapped_url = resolver.ResolveMojoURL(GURL("mojo:foo?a=b"));
  EXPECT_EQ("file:///base/foo.mojo?a=b", mapped_url.spec());
}

}  // namespace
}  // namespace test
}  // namespace shell
