def fields:
	if $ARGS.named | has("title")
	then
		$ARGS.named["title"]
	else
		[.[] | keys[]] | unique
	end;

[fields, fields as $result | .[] | [.[$result[]]] | map(if type == "string" and length > 20 then .[:20] + "..." elif type == "object" then "<<obj>>" else . end)][] | @tsv
