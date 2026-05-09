"use strict";(self.webpackChunkcli20_docs=self.webpackChunkcli20_docs||[]).push([["452"],{5741(e,s,t){t.r(s),t.d(s,{default:()=>m});var i=t(4848),r=t(5310),a=t(6497),o=t(8695),n=t(2072),c=t(1113);let l="comparisonLabel_jE6M",d=`\
#include "cli/argument.hh"
#include "cli/parser.hh"

struct Args {
  cli::Flag<"verbose", 'v'>          verbose;
  cli::StringOption<"output", 'o'>   output;
  cli::Positional<std::string,
    cli::nargs::one_or_more>         inputs;
};

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parse_or_exit<Args>(argc, argv);
}`,p=`\
CLI::App app{"my app"};
bool verbose = false;
std::string output;

app.add_flag("-v,--verbose", verbose);
app.add_option("-o,--output", output);
CLI11_PARSE(app, argc, argv);`,u=`\
struct Args {
  cli::Flag<"verbose", 'v'>        verbose;
  cli::StringOption<"output", 'o'> output;
};

const auto args =
  cli::parse_or_exit<Args>(argc, argv);`,h=[{icon:"\u{1F4E6}",title:"Header-only",description:"Drop include/ into your project. No linking, no runtime overhead."},{icon:"\u26A1",title:"C++20 native",description:"Compile-time option names, concepts, constexpr. Built for modern C++."},{icon:"\u{1F512}",title:"Type-safe schema",description:"Your CLI is a struct. No string lookups at runtime."},{icon:"\u2728",title:"Zero macros",description:"Clean modern C++. No registration macros anywhere."},{icon:"\u{1F33F}",title:"Subcommands",description:"Nested structs compose naturally. Recursive by design."},{icon:"\u{1F4D6}",title:"Auto help",description:"Add cli::Help<> help; and get full formatted help output."}];function m(){let e=(0,a.Ay)("/img/logo.png");return(0,i.jsx)(o.A,{title:"CLI20 \u2014 Define your CLI as a type, not as a builder.",description:"A header-only C++20 typed-schema command-line parser. Define your CLI as a type, not as a builder.",children:(0,i.jsxs)("main",{children:[(0,i.jsxs)("div",{className:"heroBanner_qdFl",children:[(0,i.jsx)("img",{src:e,alt:"CLI20",className:"heroLogo_U6bI"}),(0,i.jsx)("p",{children:"Define your CLI as a type, not as a builder."}),(0,i.jsx)("div",{className:"heroCode_u6tt",children:(0,i.jsx)(c.A,{language:"cpp",children:d})}),(0,i.jsxs)("div",{className:"buttons_AeoN",children:[(0,i.jsx)(r.A,{className:"button button--primary button--lg",to:"/docs/intro",children:"Get Started"}),(0,i.jsx)(r.A,{className:"button button--secondary button--lg",href:"https://github.com/CLI20-dev/cli20",children:"GitHub"})]})]}),(0,i.jsx)("section",{className:"features_cAfv",children:(0,i.jsx)("div",{className:"featuresGrid_cNCB",children:h.map(({icon:e,title:s,description:t})=>(0,i.jsxs)("div",{className:"featureCard_Jbd_",children:[(0,i.jsx)("span",{className:"featureIcon_qaBM",children:e}),(0,i.jsx)("h3",{children:s}),(0,i.jsx)("p",{children:t})]},s))})}),(0,i.jsxs)("section",{className:"comparison_UAsG",children:[(0,i.jsx)(n.A,{as:"h2",children:"CLI11 \u2192 CLI20"}),(0,i.jsxs)("div",{className:"comparisonGrid_gfIS",children:[(0,i.jsxs)("div",{children:[(0,i.jsx)("div",{className:`${l} before_e2cJ`,children:"Before (CLI11)"}),(0,i.jsx)(c.A,{language:"cpp",children:p})]}),(0,i.jsxs)("div",{children:[(0,i.jsx)("div",{className:`${l} after_ELWJ`,children:"After (CLI20)"}),(0,i.jsx)(c.A,{language:"cpp",children:u})]})]})]})]})})}}}]);