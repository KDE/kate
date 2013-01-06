





#           if a
#             return a
#           elsif $10
#             return Token.new(T_LBRA, $+)
#           elsif $11
#             return Token.new(T_RBRA, $+)
#           elsif $12
#             len = $+.to_i
#             val = @str[@pos, len]
#             @pos += len
#             return Token.new(T_LITERAL, val)
#           elsif $13
#             return Token.new(T_PLUS, $+)
#           elsif $14
#             return Token.new(T_PERCENT, $+)
#           elsif $15
#             return Token.new(T_CRLF, $+)
#           elsif $16
#             return Token.new(T_EOF, $+)
#           end
