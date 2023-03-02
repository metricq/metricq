# Integration Tests
## From Other Repo

To run the integration test from an external repo use the following step:

```yaml
      - name: Trigger Integration Workflow on runner machine
        uses: benc-uk/workflow-dispatch@v1
        with:
          workflow: metricq-integration-external-trigger.yml
          token: ${{ secrets.GITHUB_TOKEN }}
          repo: metricq/metricq
          inputs: '{"manager-ref": "master", "python-ref": "master", "cpp-ref": "master"}'
```

To create the JSON string for `inputs` you could also use the following snippet:

```
${{ toJSON({"manager-ref": "master", "python-ref": "master", "cpp-ref": "master"}) }}
```