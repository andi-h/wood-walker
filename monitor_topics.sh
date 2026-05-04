#!/usr/bin/env bash
set -u

DURATION=2
SKIP_REGEX="/parameter_events|/rosout"
SKIP_INACTIVE=1

# ---- Parse flags ----
while getopts "zd:" opt; do
  case $opt in
    z) SKIP_INACTIVE=1 ;;
    d) DURATION="$OPTARG" ;;
  esac
done

topics=$(ros2 topic list 2>/dev/null | grep -Ev "$SKIP_REGEX" || true)

printf "%-45s %-8s %-14s %-6s %-35s %-35s\n" \
"TOPIC" "Hz" "BW(last)" "msgs" "PUBLISHERS" "SUBSCRIBERS"

printf "%-45s %-8s %-14s %-6s %-35s %-35s\n" \
"---------------------------------------------" "------" "------------" "----" \
"-----------------------------------" "-----------------------------------"

for topic in $topics; do

  info="$(ros2 topic info -v "$topic" 2>/dev/null || true)"

  pub_count="$(echo "$info" | awk '/Publisher count:/ {print $3}')"
  sub_count="$(echo "$info" | awk '/Subscription count:/ {print $3}')"

  [[ -z "$pub_count" ]] && pub_count=0
  [[ -z "$sub_count" ]] && sub_count=0

  # ---- Skip inactive topics if -z is set ----
  if [[ "$SKIP_INACTIVE" -eq 1 ]]; then
    if [[ "$pub_count" -eq 0 || "$sub_count" -eq 0 ]]; then
      continue
    fi
  fi

  # ---- Extract node names ----
  publishers="$(
    echo "$info" | awk '
      BEGIN{mode=""}
      /^Publisher count:/     {mode="pub"; next}
      /^Subscription count:/  {mode="sub"; next}
      mode=="pub" && /Node name:/ {
        sub(/.*Node name:[[:space:]]*/, "", $0);
        print $0
      }
    ' | tr '\n' ',' | sed 's/,$//' || true
  )"

  subscribers="$(
    echo "$info" | awk '
      BEGIN{mode=""}
      /^Publisher count:/     {mode="pub"; next}
      /^Subscription count:/  {mode="sub"; next}
      mode=="sub" && /Node name:/ {
        sub(/.*Node name:[[:space:]]*/, "", $0);
        print $0
      }
    ' | tr '\n' ',' | sed 's/,$//' || true
  )"

  [[ -z "$publishers" ]] && publishers="-"
  [[ -z "$subscribers" ]] && subscribers="-"

  # ---- RATE ----
  rate_out="$(timeout "${DURATION}s" ros2 topic hz "$topic" 2>/dev/null || true)"
  rate="$(
    printf "%s\n" "$rate_out" \
      | grep -v '^WARNING:' \
      | grep -v '^Subscribed to' \
      | awk '/average rate:/ {r=$3} END{print r}' \
      || true
  )"
  [[ -z "$rate" ]] && rate="-"

  # ---- BANDWIDTH ----
  bw_out="$(timeout "${DURATION}s" ros2 topic bw "$topic" 2>/dev/null || true)"
  last_line="$(
    printf "%s\n" "$bw_out" \
      | grep -v '^WARNING:' \
      | grep -v '^Subscribed to' \
      | awk '/(MB\/s|KB\/s|B\/s) from [0-9]+ messages/ {line=$0} END{print line}' \
      || true
  )"

  if [[ -n "$last_line" ]]; then
    bw_val="$(echo "$last_line" | awk '{print $1, $2}')"
    msg_cnt="$(echo "$last_line" | awk '{print $4}')"
  else
    bw_val="-"
    msg_cnt="-"
  fi

  publishers_short="$(echo "$publishers" | cut -c1-35)"
  subscribers_short="$(echo "$subscribers" | cut -c1-35)"

  printf "%-45s %-8s %-14s %-6s %-35s %-35s\n" \
    "$topic" "$rate" "$bw_val" "$msg_cnt" \
    "$publishers_short" "$subscribers_short"

done
