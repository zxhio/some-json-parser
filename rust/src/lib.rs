use std::collections::HashMap;
use std::error::Error;
use std::fmt;
use std::str::FromStr;

#[derive(Debug)]
pub struct ParseError {
    pub row: usize,
    pub column: usize,
    pub desc: String,
}

impl fmt::Display for ParseError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "parse {} at pos {}:{}", self.desc, self.row, self.column)
    }
}

impl Error for ParseError {}

// #[macro_export]
macro_rules! parse_value_error {
    ($v:expr, $desc:expr) => {
        Err(ParseError {
            row: $v.row,
            column: $v.column,
            desc: $desc,
        })
    };
}

#[derive(Debug)]
pub enum Value {
    Null,
    False,
    True,
    Number { v: f64 },
    String { v: String },
    Array { v: Vec<Value> },
    Object { v: HashMap<String, Value> },
}

fn eq_value(lhs: &Value, rhs: &Value) -> bool {
    match lhs {
        Value::Null => eq_null(rhs),
        Value::False => eq_false(rhs),
        Value::True => eq_true(rhs),
        Value::Number { v } => eq_number(v, rhs),
        Value::String { v } => eq_string(v, rhs),
        Value::Array { v } => eq_array(v, rhs),
        Value::Object { v } => eq_object(v, rhs),
    }
}

fn eq_null(v: &Value) -> bool {
    match v {
        Value::Null => true,
        _ => false,
    }
}

fn eq_false(v: &Value) -> bool {
    match v {
        Value::False => true,
        _ => false,
    }
}

fn eq_true(v: &Value) -> bool {
    match v {
        Value::True => true,
        _ => false,
    }
}

fn eq_number(f: &f64, v: &Value) -> bool {
    match v {
        Value::Number { v } => v.eq(f),
        _ => false,
    }
}

fn eq_string(s: &String, v: &Value) -> bool {
    match v {
        Value::String { v } => v.eq(s),
        _ => false,
    }
}

fn eq_array(arr: &Vec<Value>, v: &Value) -> bool {
    match v {
        Value::Array { v } => v.eq(arr),
        _ => false,
    }
}

fn eq_object(obj: &HashMap<String, Value>, v: &Value) -> bool {
    match v {
        Value::Object { v } => v.eq(obj),
        _ => false,
    }
}

impl PartialEq for Value {
    fn eq(&self, other: &Self) -> bool {
        eq_value(self, other)
    }
}

pub fn type_name(v: Value) -> Option<&'static str> {
    match v {
        Value::Null => Some("null"),
        Value::False => Some("false"),
        Value::True => Some("true"),
        Value::Number { v: _ } => Some("number"),
        Value::String { v: _ } => Some("string"),
        Value::Array { v: _ } => Some("array"),
        Value::Object { v: _ } => Some("object"),
    }
}

fn value_string(v: Value) -> Option<String> {
    match v {
        Value::String { v } => Some(v),
        _ => None,
    }
}

pub struct Reader<'a> {
    context: &'a str,
    origin: &'a str,
    row: usize,
    column: usize,
}

impl<'a> Reader<'a> {
    pub fn new(c: &'a str) -> Reader {
        Reader {
            context: c,
            origin: c,
            row: 1,
            column: 1,
        }
    }

    pub fn parse(&mut self) -> Result<Value, ParseError> {
        self.context = self.origin;
        let x = self.parse_element()?;
        if self.context.len() != 0 {
            return parse_value_error!(self, format!("value not finished '{}'", self.context));
        }
        Ok(x)
    }

    fn parse_literal(&mut self, v: Value, literal: &str) -> Result<Value, ParseError> {
        if self.context.len() < literal.len() {
            return parse_value_error!(
                self,
                format!("literal '{}' expect '{}'", self.context, literal)
            );
        }

        let l = &self.context[..literal.len()];
        if literal.eq(l) {
            self.context = &self.context[literal.len()..];
            return Ok(v);
        }

        parse_value_error!(self, format!("literal not eq {}", literal))
    }

    fn parse_number(&mut self) -> Result<Value, ParseError> {
        let orig = self.context;

        // sign
        if self.peek() == Some('-') {
            self.next();
        }

        // integer, [1-9][0-9]+ | 0
        if let Some(ch) = self.peek() {
            match ch {
                '0' => {
                    self.next();
                }
                '1'..='9' => {
                    while let Some(d) = self.peek() {
                        if !d.is_ascii_digit() {
                            break;
                        }
                        self.next();
                    }
                }
                _ => {
                    return parse_value_error!(self, String::from("number integer expect '0..9'"));
                }
            }
        }

        // fractional part
        if self.peek() == Some('.') {
            while let Some(d) = self.next() {
                if !d.is_ascii_digit() {
                    break;
                }
            }
        }

        // exponent part
        match self.peek() {
            Some('e') | Some('E') => self.next(),
            Some('+') | Some('-') => self.next(),
            _ => self.peek(),
        };
        while let Some(d) = self.peek() {
            if !d.is_ascii_digit() {
                break;
            }
            self.next();
        }

        let len = orig.len() - self.context.len();
        match f64::from_str(&orig[..len]) {
            Ok(f) => Ok(Value::Number { v: f }),
            Err(e) => parse_value_error!(self, format!("'{}' to number {} error", &orig[..len], e)),
        }
    }

    fn parse_string(&mut self) -> Result<Value, ParseError> {
        if self.peek() != Some('\"') {
            return parse_value_error!(self, String::from("string start char expect '\"'"));
        }

        self.next();
        let mut s = String::new();

        while self.peek() != None {
            match self.peek() {
                Some('\"') => break,
                Some('\\') => match self.next() {
                    Some('\"') => s.push('\"'),
                    Some('\\') => s.push('\\'),
                    Some('/') => s.push('/'),
                    // Some('b') => s.push('\b'), // TODO
                    // Some('f') => s.push('\f'),
                    Some('n') => s.push('\n'),
                    Some('r') => s.push('\r'),
                    Some('t') => s.push('\t'),
                    _ => break, // TODO 4 hex digits.
                },
                Some(ch) => {
                    s.push(ch);
                }
                _ => {}
            }
            self.next();
        }

        if self.peek() != Some('\"') {
            return parse_value_error!(self, String::from("string end char expect '\"'"));
        }

        self.next();

        Ok(Value::String { v: s })
    }

    // '[' ws | elements ']'
    fn parse_array(&mut self) -> Result<Value, ParseError> {
        self.next();

        let mut arr: Vec<Value> = Vec::new();
        self.parse_whitespace();
        if self.peek() != Some(']') {
            self.parse_elements(&mut arr)?;
        }

        if self.peek() != Some(']') {
            return parse_value_error!(self, String::from("array end char expect ']'"));
        }
        self.next();

        Ok(Value::Array { v: arr })
    }

    // '{ ws | members '}'
    fn parse_object(&mut self) -> Result<Value, ParseError> {
        self.next(); // '{'

        let mut members = HashMap::new();

        self.parse_whitespace();
        if self.peek() != Some('}') {
            self.parse_members(&mut members)?;
        }

        if self.peek() != Some('}') {
            return parse_value_error!(self, String::from("object end char expect '}'"));
        }
        self.next();

        Ok(Value::Object { v: members })
    }

    fn parse_whitespace(&mut self) {
        let mut p = self.context;
        for ch in p.chars() {
            match ch {
                '\t' | '\x0C' | ' ' => {
                    p = &p[1..];
                    self.column += 1;
                }
                '\n' | '\r' => {
                    p = &p[1..];
                    self.column = 1;
                    self.row += 1;
                }
                _ => break,
            }
        }
        self.context = p;
    }

    // element ',' element
    fn parse_elements(&mut self, arr: &mut Vec<Value>) -> Result<(), ParseError> {
        let elem = self.parse_element()?;
        arr.push(elem);
        if self.peek() == Some(',') {
            self.next();
            return self.parse_elements(arr);
        }
        Ok(())
    }

    // ws value ws
    fn parse_element(&mut self) -> Result<Value, ParseError> {
        self.parse_whitespace();
        let v = self.parse_value()?;
        self.parse_whitespace();

        Ok(v)
    }

    // ws string ws ':' element
    fn parse_member(&mut self) -> Result<(Value, Value), ParseError> {
        self.parse_whitespace();
        let k = self.parse_string()?;
        self.parse_whitespace();
        if self.peek() != Some(':') {
            return parse_value_error!(self, String::from("member expect ':'"));
        }
        self.next();
        let v = self.parse_element()?;

        Ok((k, v))
    }

    // member ',' members
    fn parse_members(&mut self, objs: &mut HashMap<String, Value>) -> Result<(), ParseError> {
        let (k, v) = self.parse_member()?;
        let key = value_string(k).unwrap();
        objs.insert(key, v);

        if self.peek() == Some(',') {
            self.next();
            return self.parse_members(objs);
        }

        Ok(())
    }

    fn parse_value(&mut self) -> Result<Value, ParseError> {
        match self.peek() {
            Some('n') => self.parse_literal(Value::Null, "null"),
            Some('f') => self.parse_literal(Value::False, "false"),
            Some('t') => self.parse_literal(Value::True, "true"),
            Some('\"') => self.parse_string(),
            Some('[') => self.parse_array(),
            Some('{') => self.parse_object(),
            _ => self.parse_number(),
        }
    }

    fn peek(&mut self) -> Option<char> {
        let mut c = self.context.chars();
        if let Some(ch) = c.next() {
            return Some(ch);
        }

        None
    }

    fn next(&mut self) -> Option<char> {
        let mut c = self.context.chars();
        if let Some(_) = c.next() {
            self.context = c.as_str();
            self.column += 1;
            return self.peek();
        }

        None
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_value_name() {
        assert_eq!(Some("null"), type_name(Value::Null));
        assert_eq!(Some("false"), type_name(Value::False));
        assert_eq!(Some("true"), type_name(Value::True));
        assert_eq!(Some("number"), type_name(Value::Number { v: 64.0 }));
        assert_eq!(
            Some("string"),
            type_name(Value::String {
                v: String::from("str")
            })
        );
        assert_eq!(
            Some("array"),
            type_name(Value::Array {
                v: vec![Value::Null, Value::False]
            })
        );
        assert_eq!(
            Some("object"),
            type_name(Value::Object { v: HashMap::new() })
        );
    }

    #[test]
    fn test_eq() {
        let n = Value::Null;
        // assert!(eq_literal(Value::Null, &n));
        assert!(eq_value(&Value::Null, &n));
        assert_eq!(&Value::Null, &n);

        let f = Value::False;
        // assert!(eq_literal(Value::False, &f));
        assert!(eq_value(&Value::False, &f));
        assert_eq!(&Value::False, &f);

        let t = Value::True;
        // assert!(eq_literal(Value::True, &f));
        assert!(eq_value(&Value::True, &t));
        assert_eq!(&Value::True, &t);

        let num = Value::Number { v: 3.14159 };
        assert!(eq_number(&3.14159, &num));
        assert!(eq_value(&Value::Number { v: 3.14159 }, &num));
        assert_eq!(&Value::Number { v: 3.14159 }, &num);

        let arr = Value::Array {
            v: vec![Value::Null, Value::False],
        };
        assert!(eq_array(&vec![Value::Null, Value::False], &arr));
        assert!(eq_value(
            &Value::Array {
                v: vec![Value::Null, Value::False]
            },
            &arr
        ));
        assert_eq!(
            &Value::Array {
                v: vec![Value::Null, Value::False]
            },
            &arr
        );

        let obj = Value::Object { v: HashMap::new() };
        assert!(eq_object(&HashMap::new(), &obj));
        assert!(eq_value(&Value::Object { v: HashMap::new() }, &obj));
        assert_eq!(&Value::Object { v: HashMap::new() }, &obj);
    }

    #[test]
    fn test_peek_next() {
        let mut r = Reader::new("{\"n\":1}");
        assert_eq!(Some('{'), r.peek());
        assert_eq!(Some('\"'), r.next());
        assert_eq!(Some('n'), r.next());
        assert_eq!(Some('\"'), r.next());
        assert_eq!(Some(':'), r.next());
        assert_eq!(Some('1'), r.next());
        assert_eq!(Some('}'), r.next());
        assert_eq!(None, r.next());
        assert_eq!(None, r.next());
    }

    #[test]
    fn test_parse_whitespace() {
        let mut r = Reader::new("  {}");
        r.parse_whitespace();
        assert_eq!("{}", r.context);
    }

    #[test]
    fn test_parse_null() {
        let mut r = Reader::new("null");
        assert!(r.parse_literal(Value::Null, "null").is_ok());
        assert!(r.parse().is_ok());
        assert_eq!(r.parse().unwrap(), Value::Null);
    }

    #[test]
    fn test_parse_false() {
        let mut r = Reader::new("false");
        assert!(r.parse_literal(Value::False, "false").is_ok());
        assert!(r.parse().is_ok());
        assert_eq!(r.parse().unwrap(), Value::False);
    }

    #[test]
    fn test_parse_true() {
        let mut r = Reader::new("true");
        let x = r.parse_literal(Value::True, "true");
        assert!(x.is_ok(), "{}", x.unwrap_err().desc);
        assert_eq!(r.parse().unwrap(), Value::True);
    }

    #[test]
    fn test_parse_number() {
        let mut r = Reader::new("0");
        let x = r.parse_number();
        assert!(x.is_ok(), "{}", x.unwrap_err().desc);
        assert_eq!(r.parse().unwrap(), Value::Number { v: 0.0 });

        let mut r1 = Reader::new("-0.1");
        let x1 = r1.parse_number();
        assert!(x1.is_ok(), "{}", x1.unwrap_err().desc);
        assert_eq!(r1.parse().unwrap(), Value::Number { v: -0.1 });

        let mut r2 = Reader::new("0.");
        let x2 = r2.parse_number();
        assert!(x2.is_ok(), "{}", x2.unwrap_err().desc);
        assert_eq!(r2.parse().unwrap(), Value::Number { v: 0.0 });

        let mut r3 = Reader::new("12345");
        let x3 = r3.parse_number();
        assert!(x3.is_ok(), "{}", x3.unwrap_err().desc);
        assert_eq!(r3.parse().unwrap(), Value::Number { v: 12345.0 });

        let mut r4 = Reader::new("-12345");
        let x4 = r4.parse_number();
        assert!(x4.is_ok(), "{}", x4.unwrap_err().desc);
        assert_eq!(r4.parse().unwrap(), Value::Number { v: -12345.0 });
    }

    #[test]
    fn test_prase_string() {
        let mut r = Reader::new("\"\"");
        let x = r.parse_string();
        assert!(x.is_ok(), "{}", x.unwrap_err().desc);
        assert_eq!(
            x.unwrap(),
            Value::String {
                v: String::from("")
            }
        );

        let mut r1 = Reader::new("\"string\"");
        let x1 = r1.parse_string();
        assert!(x1.is_ok(), "{}", x1.unwrap_err().desc);
        assert_eq!(
            x1.unwrap(),
            Value::String {
                v: String::from("string")
            }
        );

        let mut r2 = Reader::new("\"\\\"\"");
        let x2 = r2.parse_string();
        assert!(x2.is_ok(), "{}", x2.unwrap_err().desc);
        assert_eq!(
            x2.unwrap(),
            Value::String {
                v: String::from("\"")
            }
        );

        let mut r3 = Reader::new("\"\\\"\\\\\\/\\n\\r\\t/\"");
        let x3 = r3.parse_string();
        assert!(x3.is_ok(), "{}", x3.unwrap_err().desc);
        assert_eq!(
            x3.unwrap(),
            Value::String {
                v: String::from("\"\\/\n\r\t/")
            }
        );
    }

    #[test]
    fn test_parse_array() {
        let mut r = Reader::new("[]");
        let x = r.parse_array();
        assert!(x.is_ok(), "{}", x.unwrap_err().desc);
        assert_eq!(x.unwrap(), Value::Array { v: vec![] });

        let mut r1 = Reader::new("[false, true,null]");
        let x1 = r1.parse_array();
        assert!(x1.is_ok(), "{}", x1.unwrap_err().desc);
        assert_eq!(
            x1.unwrap(),
            Value::Array {
                v: vec![Value::False, Value::True, Value::Null]
            }
        );

        let mut r2 = Reader::new("[[false,true, false], [null]]");
        let x2 = r2.parse_array();
        assert!(x2.is_ok(), "{}", x2.unwrap_err().desc);
        assert_eq!(
            x2.unwrap(),
            Value::Array {
                v: vec![
                    Value::Array {
                        v: vec![Value::False, Value::True, Value::False]
                    },
                    Value::Array {
                        v: vec![Value::Null]
                    }
                ]
            }
        );
    }

    #[test]
    fn test_parse_object() {
        let mut r = Reader::new("{}");
        let x = r.parse_object();
        assert!(x.is_ok(), "{}", x.unwrap_err().desc);
        assert_eq!(x.unwrap(), Value::Object { v: HashMap::new() });

        let mut r1 = Reader::new("{\"hello\":true}");
        let mut m1 = HashMap::new();
        m1.insert("hello".to_string(), Value::True);
        let x1 = r1.parse_object();
        assert!(x1.is_ok(), "{}", x1.unwrap_err().desc);
        assert_eq!(x1.unwrap(), Value::Object { v: m1 });

        let mut r2 =
            Reader::new("{\"name\":\"zxh\",\"option\":[true,false,3.14159],\"open\":null}");
        let mut m2 = HashMap::new();
        m2.insert(
            "name".to_string(),
            Value::String {
                v: "zxh".to_string(),
            },
        );
        m2.insert("open".to_string(), Value::Null);
        m2.insert(
            "option".to_string(),
            Value::Array {
                v: vec![Value::True, Value::False, Value::Number { v: 3.14159 }],
            },
        );
        let x2 = r2.parse_object();
        assert!(x2.is_ok(), "{}", x2.unwrap_err().desc);
        assert_eq!(x2.unwrap(), Value::Object { v: m2 });
    }
}
